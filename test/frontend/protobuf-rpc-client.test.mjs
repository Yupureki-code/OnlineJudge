import assert from "node:assert/strict";
import { createRequire } from "node:module";
import test from "node:test";

import {
  ProtobufApplicationError,
  ProtobufDecodeError,
  ProtobufHttpError,
  ProtobufRpcClient,
  ProtobufRpcContentTypeError,
  ProtobufRpcTimeoutError,
  ProtobufResponseTooLargeError,
  uint64ToDecimalString,
} from "../../src/wwwroot_brpc/js/protobuf-rpc-client.js";
import { oj } from "../../src/wwwroot_brpc/js/generated/oj-proto.js";

const require = createRequire(new URL("../../src/wwwroot_brpc/package.json", import.meta.url));
const protobuf = require("protobufjs");
const Long = require("long");
protobuf.util.Long = Long;
protobuf.configure();

const Request = new protobuf.Type("Request")
  .add(new protobuf.Field("submissionId", 1, "uint64"))
  .add(new protobuf.Field("source", 2, "string"));
const ResponseMessage = new protobuf.Type("ResponseMessage")
  .add(new protobuf.Field("submissionId", 1, "uint64"))
  .add(new protobuf.Field("status", 2, "string"));
const ErrorMessage = new protobuf.Type("ErrorMessage")
  .add(new protobuf.Field("code", 1, "string"))
  .add(new protobuf.Field("message", 2, "string"));

function protobufResponse(type, value, init = {}) {
  const body = type.encode(type.fromObject(value)).finish();
  const headers = new Headers(init.headers);
  headers.set("content-type", "application/x-protobuf");
  return new Response(body, { ...init, headers });
}

test("encodes requests and decodes responses without narrowing uint64", async () => {
  const submissionId = "18446744073709551615";
  const fetchMock = async (url, init) => {
    assert.equal(url, "/oj.biz.SubmissionService/GetSubmission");
    assert.equal(init.method, "POST");
    assert.equal(init.credentials, "same-origin");
    assert.equal(init.headers.get("content-type"), "application/x-protobuf");
    assert.equal(init.headers.get("accept"), "application/x-protobuf");

    const request = Request.decode(init.body);
    assert.equal(Long.isLong(request.submissionId), true);
    assert.equal(request.submissionId.toString(), submissionId);
    assert.equal(request.source, "int main() {}");
    return protobufResponse(ResponseMessage, { submissionId, status: "PENDING" });
  };

  const client = new ProtobufRpcClient({ fetch: fetchMock });
  const response = await client.call(
    "/oj.biz.SubmissionService/GetSubmission",
    Request,
    ResponseMessage,
    { submissionId, source: "int main() {}" },
  );

  assert.equal(Long.isLong(response.submissionId), true);
  assert.equal(uint64ToDecimalString(response.submissionId), submissionId);
  assert.equal(response.status, "PENDING");
  assert.throws(() => uint64ToDecimalString(42), /JavaScript numbers/);
});

test("generated Phase 3 messages preserve the full uint64 range", () => {
  const submissionId = "18446744073709551615";
  const message = oj.common.Submission.fromObject({
    submissionId,
    status: oj.common.SubmissionStatus.SUBMISSION_STATUS_QUEUED,
  });
  const decoded = oj.common.Submission.decode(oj.common.Submission.encode(message).finish());

  assert.equal(Long.isLong(decoded.submissionId), true);
  assert.equal(uint64ToDecimalString(decoded.submissionId), submissionId);
});

test("rejects uint64 values that protobuf.js would silently coerce", async () => {
  const client = new ProtobufRpcClient({
    fetch: async () => assert.fail("invalid requests must fail before fetch"),
  });
  for (const submissionId of [-1, "-1", "1.5", "18446744073709551616"]) {
    await assert.rejects(
      client.call(
        "/oj.biz.SubmissionService/GetSubmission",
        oj.biz.GetSubmissionRequest,
        oj.biz.GetSubmissionResponse,
        { submissionId },
      ),
      /uint64 request values/,
    );
  }
});

test("rejects signed int64 values that protobuf.js would silently coerce", async () => {
  const client = new ProtobufRpcClient({
    fetch: async () => assert.fail("invalid requests must fail before fetch"),
  });
  for (const startTime of [9007199254740993, "1.5", "9223372036854775808", "-9223372036854775809"]) {
    await assert.rejects(
      client.call(
        "/oj.biz.AdminService/GetAuditLogs",
        oj.biz.GetAuditLogsRequest,
        oj.biz.GetAuditLogsResponse,
        { startTime },
      ),
      /int64 request values/,
    );
  }
});

test("generated service metadata uses native brpc paths", () => {
  assert.equal(
    oj.biz.SubmissionService.prototype.getSubmission.path,
    "/oj.biz.SubmissionService/GetSubmission",
  );
  assert.equal(
    oj.biz.UserService.prototype.getCurrentUser.path,
    "/oj.biz.UserService/GetCurrentUser",
  );
});

test("decodes protobuf error bodies on non-200 responses", async () => {
  const fetchMock = async () => protobufResponse(
    ErrorMessage,
    { code: "UNAUTHENTICATED", message: "login required" },
    { status: 401, statusText: "Unauthorized" },
  );
  const client = new ProtobufRpcClient({ fetch: fetchMock });

  await assert.rejects(
    client.call("/oj.biz.UserService/GetCurrentUser", Request, ResponseMessage, {}, { errorType: ErrorMessage }),
    (error) => {
      assert.equal(error instanceof ProtobufHttpError, true);
      assert.equal(error.status, 401);
      assert.equal(error.errorResponse.code, "UNAUTHENTICATED");
      assert.equal(error.errorResponse.message, "login required");
      return true;
    },
  );
});

test("rejects protobuf application errors returned with HTTP 200", async () => {
  const client = new ProtobufRpcClient({
    fetch: async () => protobufResponse(oj.common.EmptyResponse, {
      status: {
        code: oj.common.ErrorCode.ERROR_CODE_UNAUTHENTICATED,
        message: "login required",
        retryable: false,
      },
    }),
  });

  await assert.rejects(
    client.call(
      "/oj.biz.UserService/Logout",
      oj.common.EmptyRequest,
      oj.common.EmptyResponse,
    ),
    (error) => {
      assert.equal(error instanceof ProtobufApplicationError, true);
      assert.equal(error.code, oj.common.ErrorCode.ERROR_CODE_UNAUTHENTICATED);
      assert.equal(error.message, "login required");
      return true;
    },
  );
});

test("rejects successful responses with a non-protobuf content type", async () => {
  const client = new ProtobufRpcClient({
    fetch: async () => new Response("not protobuf", {
      status: 200,
      headers: { "content-type": "text/plain" },
    }),
  });

  await assert.rejects(
    client.call("/oj.biz.UserService/GetCurrentUser", Request, ResponseMessage),
    ProtobufRpcContentTypeError,
  );
});

test("reports malformed protobuf response bodies", async () => {
  const client = new ProtobufRpcClient({
    fetch: async () => new Response(Uint8Array.of(0x80), {
      status: 200,
      headers: { "content-type": "application/x-protobuf; charset=binary" },
    }),
  });

  await assert.rejects(
    client.call("/oj.biz.UserService/GetCurrentUser", Request, ResponseMessage),
    ProtobufDecodeError,
  );
});

test("aborts requests after the configured timeout", async () => {
  const fetchMock = (_url, { signal }) => new Promise((_resolve, reject) => {
    signal.addEventListener("abort", () => reject(signal.reason), { once: true });
  });
  const client = new ProtobufRpcClient({ fetch: fetchMock, timeoutMs: 10 });

  await assert.rejects(
    client.call("/oj.biz.UserService/GetCurrentUser", Request, ResponseMessage),
    ProtobufRpcTimeoutError,
  );
});

test("enforces timeout even when fetch ignores AbortSignal", async () => {
  const client = new ProtobufRpcClient({
    fetch: async () => new Promise((resolve) => {
      setTimeout(() => resolve(protobufResponse(ResponseMessage, {})), 50);
    }),
    timeoutMs: 5,
  });

  await assert.rejects(
    client.call("/oj.biz.UserService/GetCurrentUser", Request, ResponseMessage),
    ProtobufRpcTimeoutError,
  );
});

test("rejects oversized responses and timer values that browsers clamp", async () => {
  const client = new ProtobufRpcClient({
    fetch: async () => new Response(Uint8Array.of(1, 2, 3), {
      headers: { "content-type": "application/x-protobuf" },
    }),
    maxResponseBytes: 2,
  });
  await assert.rejects(
    client.call("/oj.biz.UserService/GetCurrentUser", Request, ResponseMessage),
    ProtobufResponseTooLargeError,
  );
  assert.throws(
    () => new ProtobufRpcClient({ fetch: async () => {}, timeoutMs: 2_147_483_648 }),
    /timeoutMs/,
  );
});

test("accepts caller AbortSignal and rejects legacy API paths", async () => {
  const controller = new AbortController();
  const fetchMock = (_url, { signal }) => new Promise((_resolve, reject) => {
    signal.addEventListener("abort", () => reject(signal.reason), { once: true });
  });
  const client = new ProtobufRpcClient({ fetch: fetchMock });
  const request = client.call(
    "/oj.biz.UserService/GetCurrentUser",
    Request,
    ResponseMessage,
    {},
    { signal: controller.signal },
  );
  controller.abort(new Error("cancelled by caller"));

  await assert.rejects(request, /cancelled by caller/);
  await assert.rejects(
    client.call("/api/user", Request, ResponseMessage),
    /native brpc RPC path/,
  );
});
