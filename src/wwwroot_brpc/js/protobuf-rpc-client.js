const PROTOBUF_CONTENT_TYPE = "application/x-protobuf";
const NATIVE_BRPC_PATH = /^\/(?:[A-Za-z_]\w*\.)*[A-Za-z_]\w*Service\/[A-Za-z_]\w*$/;

export class ProtobufRpcError extends Error {
  constructor(message, options = {}) {
    super(message);
    this.name = new.target.name;
    if (options.cause !== undefined) {
      this.cause = options.cause;
    }
  }
}

export class ProtobufHttpError extends ProtobufRpcError {
  constructor(response, body, errorResponse, decodeError) {
    super(`Protobuf RPC failed with HTTP ${response.status} ${response.statusText}`.trim());
    this.status = response.status;
    this.statusText = response.statusText;
    this.body = body;
    this.errorResponse = errorResponse;
    this.decodeError = decodeError;
  }
}

export class ProtobufRpcContentTypeError extends ProtobufRpcError {
  constructor(contentType) {
    super(`Expected ${PROTOBUF_CONTENT_TYPE} response, received ${contentType || "no content type"}`);
    this.contentType = contentType;
  }
}

export class ProtobufEncodeError extends ProtobufRpcError {}
export class ProtobufDecodeError extends ProtobufRpcError {}
export class ProtobufRpcTimeoutError extends ProtobufRpcError {}
export class ProtobufApplicationError extends ProtobufRpcError {
  constructor(status, response) {
    super(status.message || `Protobuf RPC application error ${status.code}`);
    this.code = status.code;
    this.retryable = status.retryable === true;
    this.response = response;
  }
}
export class ProtobufResponseTooLargeError extends ProtobufRpcError {}

const MAX_UINT64 = 18_446_744_073_709_551_615n;
const MAX_INT64 = 9_223_372_036_854_775_807n;
const MIN_INT64 = -9_223_372_036_854_775_808n;
const MAX_TIMER_DELAY_MS = 2_147_483_647;
const ERROR_CODE_OK = 1;

function isProtobufContentType(contentType) {
  return typeof contentType === "string"
    && contentType.split(";", 1)[0].trim().toLowerCase() === PROTOBUF_CONTENT_TYPE;
}

function assertMessageType(messageType, operation) {
  if (!messageType || typeof messageType[operation] !== "function") {
    throw new TypeError(`Protobuf message type must provide ${operation}()`);
  }
}

function createMessage(messageType, value) {
  if (typeof messageType.fromObject === "function") {
    const source = value ?? {};
    const message = messageType.fromObject(source);
    validateLongInputs(source, message);
    return message;
  }
  if (typeof messageType.create === "function") {
    return messageType.create(value ?? {});
  }
  return value ?? {};
}

function isLongValue(value) {
  return value !== null
    && typeof value === "object"
    && typeof value.unsigned === "boolean"
    && Number.isInteger(value.low)
    && Number.isInteger(value.high)
    && typeof value.toString === "function";
}

function validateLongValue(value, unsigned) {
  if (typeof value === "number") {
    throw new TypeError(`${unsigned ? "uint64" : "int64"} request values must not be JavaScript numbers`);
  }

  const decimal = typeof value === "string" || typeof value === "bigint"
    ? String(value)
    : value && typeof value === "object" && typeof value.toString === "function"
      ? value.toString(10)
      : "";
  const validSyntax = unsigned ? /^\d+$/.test(decimal) : /^-?\d+$/.test(decimal);
  if (!validSyntax) {
    throw new TypeError(`${unsigned ? "uint64" : "int64"} request values must be decimal integers in range`);
  }
  const numeric = BigInt(decimal);
  if ((unsigned && numeric > MAX_UINT64)
      || (!unsigned && (numeric < MIN_INT64 || numeric > MAX_INT64))) {
    throw new TypeError(`${unsigned ? "uint64" : "int64"} request values must be decimal integers in range`);
  }
}

function validateLongInputs(source, converted) {
  if (isLongValue(converted)) {
    validateLongValue(source, converted.unsigned);
    return;
  }
  if (Array.isArray(converted)) {
    for (let index = 0; index < converted.length; ++index) {
      validateLongInputs(source?.[index], converted[index]);
    }
    return;
  }
  if (!converted || typeof converted !== "object" || !source || typeof source !== "object") {
    return;
  }
  for (const [key, convertedValue] of Object.entries(converted)) {
    if (Object.prototype.hasOwnProperty.call(source, key)) {
      validateLongInputs(source[key], convertedValue);
    }
  }
}

function encodeRequest(messageType, value) {
  assertMessageType(messageType, "encode");

  try {
    const message = createMessage(messageType, value);
    if (typeof messageType.verify === "function") {
      const verificationError = messageType.verify(message);
      if (verificationError) {
        throw new TypeError(verificationError);
      }
    }
    return messageType.encode(message).finish();
  } catch (cause) {
    throw new ProtobufEncodeError(`Failed to encode protobuf request: ${cause.message}`, { cause });
  }
}

function decodeResponse(messageType, body) {
  assertMessageType(messageType, "decode");

  try {
    return messageType.decode(body);
  } catch (cause) {
    throw new ProtobufDecodeError(`Failed to decode protobuf response: ${cause.message}`, { cause });
  }
}

function assertNativeBrpcPath(path) {
  if (typeof path !== "string" || !NATIVE_BRPC_PATH.test(path)) {
    throw new TypeError(
      `Expected a native brpc RPC path such as /oj.biz.UserService/GetCurrentUser, received ${String(path)}`,
    );
  }
}

function validatePositiveTimeout(timeoutMs) {
  if (!Number.isInteger(timeoutMs) || timeoutMs <= 0 || timeoutMs > MAX_TIMER_DELAY_MS) {
    throw new TypeError(`timeoutMs must be an integer between 1 and ${MAX_TIMER_DELAY_MS}`);
  }
}

async function readResponseBody(response, maxResponseBytes) {
  const contentLength = Number(response.headers.get("content-length"));
  if (Number.isFinite(contentLength) && contentLength > maxResponseBytes) {
    throw new ProtobufResponseTooLargeError(
      `Protobuf RPC response exceeds ${maxResponseBytes} bytes`,
    );
  }

  if (!response.body || typeof response.body.getReader !== "function") {
    const body = new Uint8Array(await response.arrayBuffer());
    if (body.byteLength > maxResponseBytes) {
      throw new ProtobufResponseTooLargeError(
        `Protobuf RPC response exceeds ${maxResponseBytes} bytes`,
      );
    }
    return body;
  }

  const reader = response.body.getReader();
  const chunks = [];
  let length = 0;
  while (true) {
    const { done, value } = await reader.read();
    if (done) break;
    length += value.byteLength;
    if (length > maxResponseBytes) {
      await reader.cancel();
      throw new ProtobufResponseTooLargeError(
        `Protobuf RPC response exceeds ${maxResponseBytes} bytes`,
      );
    }
    chunks.push(value);
  }
  const body = new Uint8Array(length);
  let offset = 0;
  for (const chunk of chunks) {
    body.set(chunk, offset);
    offset += chunk.byteLength;
  }
  return body;
}

export function uint64ToDecimalString(value) {
  if (typeof value === "number") {
    throw new TypeError("uint64 values must not be converted through JavaScript numbers");
  }

  const decimal = typeof value === "string" || typeof value === "bigint"
    ? String(value)
    : value && typeof value === "object" && typeof value.toString === "function"
      ? value.toString(10)
      : "";

  if (!/^\d+$/.test(decimal)) {
    throw new TypeError("Expected a uint64 Long, bigint, or unsigned decimal string");
  }
  return decimal;
}

export class ProtobufRpcClient {
  constructor({
    fetch: fetchImplementation = globalThis.fetch,
    timeoutMs = 15_000,
    maxResponseBytes = 4 * 1024 * 1024,
  } = {}) {
    if (typeof fetchImplementation !== "function") {
      throw new TypeError("A fetch implementation is required");
    }
    validatePositiveTimeout(timeoutMs);
    if (!Number.isInteger(maxResponseBytes) || maxResponseBytes <= 0) {
      throw new TypeError("maxResponseBytes must be a positive integer");
    }

    this.fetch = fetchImplementation;
    this.timeoutMs = timeoutMs;
    this.maxResponseBytes = maxResponseBytes;
  }

  async call(path, requestType, responseType, request = {}, options = {}) {
    assertNativeBrpcPath(path);
    const body = encodeRequest(requestType, request);
    const timeoutMs = options.timeoutMs ?? this.timeoutMs;
    validatePositiveTimeout(timeoutMs);
    const maxResponseBytes = options.maxResponseBytes ?? this.maxResponseBytes;
    if (!Number.isInteger(maxResponseBytes) || maxResponseBytes <= 0) {
      throw new TypeError("maxResponseBytes must be a positive integer");
    }

    const headers = new Headers(options.headers);
    headers.set("content-type", PROTOBUF_CONTENT_TYPE);
    headers.set("accept", PROTOBUF_CONTENT_TYPE);

    const controller = new AbortController();
    const abortFromCaller = () => controller.abort(options.signal.reason);
    if (options.signal?.aborted) {
      abortFromCaller();
    } else {
      options.signal?.addEventListener("abort", abortFromCaller, { once: true });
    }

    let timeout;
    let timeoutError;
    const timeoutPromise = new Promise((_resolve, reject) => {
      timeout = setTimeout(() => {
        timeoutError = new ProtobufRpcTimeoutError(
          `Protobuf RPC timed out after ${timeoutMs} ms`,
        );
        controller.abort(timeoutError);
        reject(timeoutError);
      }, timeoutMs);
    });

    try {
      const requestPromise = (async () => {
        const response = await this.fetch(path, {
          method: "POST",
          headers,
          body,
          credentials: "same-origin",
          signal: controller.signal,
        });
        const contentType = response.headers.get("content-type");
        const responseBody = await readResponseBody(response, maxResponseBytes);

        if (!response.ok) {
          let errorResponse;
          let decodeError;
          if (options.errorType && isProtobufContentType(contentType)) {
            try {
              errorResponse = decodeResponse(options.errorType, responseBody);
            } catch (error) {
              decodeError = error;
            }
          }
          throw new ProtobufHttpError(response, responseBody, errorResponse, decodeError);
        }

        if (!isProtobufContentType(contentType)) {
          throw new ProtobufRpcContentTypeError(contentType);
        }
        const decoded = decodeResponse(responseType, responseBody);
        if (decoded?.status && typeof decoded.status === "object"
            && decoded.status.code !== ERROR_CODE_OK) {
          throw new ProtobufApplicationError(decoded.status, decoded);
        }
        return decoded;
      })();
      return await Promise.race([requestPromise, timeoutPromise]);
    } catch (cause) {
      if (timeoutError && (cause === timeoutError || controller.signal.reason === timeoutError)) {
        throw timeoutError;
      }
      throw cause;
    } finally {
      clearTimeout(timeout);
      options.signal?.removeEventListener("abort", abortFromCaller);
    }
  }
}

export function createProtobufRpcClient(options) {
  return new ProtobufRpcClient(options);
}
