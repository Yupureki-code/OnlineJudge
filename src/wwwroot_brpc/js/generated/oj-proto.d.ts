import * as $protobuf from "protobufjs";
import Long = require("long");

/** Namespace oj. */
export namespace oj {

    /** Namespace biz. */
    namespace biz {

        /**
         * Properties of a SendVerificationCodeRequest.
         * @deprecated Use oj.biz.SendVerificationCodeRequest.$Properties instead.
         */
        interface ISendVerificationCodeRequest extends oj.biz.SendVerificationCodeRequest.$Properties {
        }

        /** Represents a SendVerificationCodeRequest. */
        class SendVerificationCodeRequest {

            /**
             * Constructs a new SendVerificationCodeRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.SendVerificationCodeRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** SendVerificationCodeRequest email. */
            email: string;

            /**
             * Creates a new SendVerificationCodeRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns SendVerificationCodeRequest instance
             */
            static create(properties: oj.biz.SendVerificationCodeRequest.$Shape): oj.biz.SendVerificationCodeRequest & oj.biz.SendVerificationCodeRequest.$Shape;
            static create(properties?: oj.biz.SendVerificationCodeRequest.$Properties): oj.biz.SendVerificationCodeRequest;

            /**
             * Encodes the specified SendVerificationCodeRequest message. Does not implicitly {@link oj.biz.SendVerificationCodeRequest.verify|verify} messages.
             * @param message SendVerificationCodeRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.SendVerificationCodeRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified SendVerificationCodeRequest message, length delimited. Does not implicitly {@link oj.biz.SendVerificationCodeRequest.verify|verify} messages.
             * @param message SendVerificationCodeRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.SendVerificationCodeRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a SendVerificationCodeRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.SendVerificationCodeRequest & oj.biz.SendVerificationCodeRequest.$Shape} SendVerificationCodeRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.SendVerificationCodeRequest & oj.biz.SendVerificationCodeRequest.$Shape;

            /**
             * Decodes a SendVerificationCodeRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.SendVerificationCodeRequest & oj.biz.SendVerificationCodeRequest.$Shape} SendVerificationCodeRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.SendVerificationCodeRequest & oj.biz.SendVerificationCodeRequest.$Shape;

            /**
             * Verifies a SendVerificationCodeRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a SendVerificationCodeRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns SendVerificationCodeRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.SendVerificationCodeRequest;

            /**
             * Creates a plain object from a SendVerificationCodeRequest message. Also converts values to other types if specified.
             * @param message SendVerificationCodeRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.SendVerificationCodeRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this SendVerificationCodeRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for SendVerificationCodeRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace SendVerificationCodeRequest {

            /** Properties of a SendVerificationCodeRequest. */
            interface $Properties {

                /** SendVerificationCodeRequest email */
                email?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a SendVerificationCodeRequest. */
            type $Shape = oj.biz.SendVerificationCodeRequest.$Properties;
        }

        /**
         * Properties of a SendVerificationCodeResponse.
         * @deprecated Use oj.biz.SendVerificationCodeResponse.$Properties instead.
         */
        interface ISendVerificationCodeResponse extends oj.biz.SendVerificationCodeResponse.$Properties {
        }

        /** Represents a SendVerificationCodeResponse. */
        class SendVerificationCodeResponse {

            /**
             * Constructs a new SendVerificationCodeResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.SendVerificationCodeResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** SendVerificationCodeResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** SendVerificationCodeResponse retryAfterSeconds. */
            retryAfterSeconds: number;

            /**
             * Creates a new SendVerificationCodeResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns SendVerificationCodeResponse instance
             */
            static create(properties: oj.biz.SendVerificationCodeResponse.$Shape): oj.biz.SendVerificationCodeResponse & oj.biz.SendVerificationCodeResponse.$Shape;
            static create(properties?: oj.biz.SendVerificationCodeResponse.$Properties): oj.biz.SendVerificationCodeResponse;

            /**
             * Encodes the specified SendVerificationCodeResponse message. Does not implicitly {@link oj.biz.SendVerificationCodeResponse.verify|verify} messages.
             * @param message SendVerificationCodeResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.SendVerificationCodeResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified SendVerificationCodeResponse message, length delimited. Does not implicitly {@link oj.biz.SendVerificationCodeResponse.verify|verify} messages.
             * @param message SendVerificationCodeResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.SendVerificationCodeResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a SendVerificationCodeResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.SendVerificationCodeResponse & oj.biz.SendVerificationCodeResponse.$Shape} SendVerificationCodeResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.SendVerificationCodeResponse & oj.biz.SendVerificationCodeResponse.$Shape;

            /**
             * Decodes a SendVerificationCodeResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.SendVerificationCodeResponse & oj.biz.SendVerificationCodeResponse.$Shape} SendVerificationCodeResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.SendVerificationCodeResponse & oj.biz.SendVerificationCodeResponse.$Shape;

            /**
             * Verifies a SendVerificationCodeResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a SendVerificationCodeResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns SendVerificationCodeResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.SendVerificationCodeResponse;

            /**
             * Creates a plain object from a SendVerificationCodeResponse message. Also converts values to other types if specified.
             * @param message SendVerificationCodeResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.SendVerificationCodeResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this SendVerificationCodeResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for SendVerificationCodeResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace SendVerificationCodeResponse {

            /** Properties of a SendVerificationCodeResponse. */
            interface $Properties {

                /** SendVerificationCodeResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** SendVerificationCodeResponse retryAfterSeconds */
                retryAfterSeconds?: (number|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a SendVerificationCodeResponse. */
            type $Shape = oj.biz.SendVerificationCodeResponse.$Properties;
        }

        /**
         * Properties of a RegisterRequest.
         * @deprecated Use oj.biz.RegisterRequest.$Properties instead.
         */
        interface IRegisterRequest extends oj.biz.RegisterRequest.$Properties {
        }

        /** Represents a RegisterRequest. */
        class RegisterRequest {

            /**
             * Constructs a new RegisterRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.RegisterRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** RegisterRequest email. */
            email: string;

            /** RegisterRequest verificationCode. */
            verificationCode: string;

            /** RegisterRequest name. */
            name: string;

            /** RegisterRequest password. */
            password: string;

            /**
             * Creates a new RegisterRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns RegisterRequest instance
             */
            static create(properties: oj.biz.RegisterRequest.$Shape): oj.biz.RegisterRequest & oj.biz.RegisterRequest.$Shape;
            static create(properties?: oj.biz.RegisterRequest.$Properties): oj.biz.RegisterRequest;

            /**
             * Encodes the specified RegisterRequest message. Does not implicitly {@link oj.biz.RegisterRequest.verify|verify} messages.
             * @param message RegisterRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.RegisterRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified RegisterRequest message, length delimited. Does not implicitly {@link oj.biz.RegisterRequest.verify|verify} messages.
             * @param message RegisterRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.RegisterRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a RegisterRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.RegisterRequest & oj.biz.RegisterRequest.$Shape} RegisterRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.RegisterRequest & oj.biz.RegisterRequest.$Shape;

            /**
             * Decodes a RegisterRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.RegisterRequest & oj.biz.RegisterRequest.$Shape} RegisterRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.RegisterRequest & oj.biz.RegisterRequest.$Shape;

            /**
             * Verifies a RegisterRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a RegisterRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns RegisterRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.RegisterRequest;

            /**
             * Creates a plain object from a RegisterRequest message. Also converts values to other types if specified.
             * @param message RegisterRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.RegisterRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this RegisterRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for RegisterRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace RegisterRequest {

            /** Properties of a RegisterRequest. */
            interface $Properties {

                /** RegisterRequest email */
                email?: (string|null);

                /** RegisterRequest verificationCode */
                verificationCode?: (string|null);

                /** RegisterRequest name */
                name?: (string|null);

                /** RegisterRequest password */
                password?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a RegisterRequest. */
            type $Shape = oj.biz.RegisterRequest.$Properties;
        }

        /**
         * Properties of an AuthResponse.
         * @deprecated Use oj.biz.AuthResponse.$Properties instead.
         */
        interface IAuthResponse extends oj.biz.AuthResponse.$Properties {
        }

        /** Represents an AuthResponse. */
        class AuthResponse {

            /**
             * Constructs a new AuthResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AuthResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AuthResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** AuthResponse user. */
            user?: (oj.common.User.$Properties|null);

            /**
             * Creates a new AuthResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AuthResponse instance
             */
            static create(properties: oj.biz.AuthResponse.$Shape): oj.biz.AuthResponse & oj.biz.AuthResponse.$Shape;
            static create(properties?: oj.biz.AuthResponse.$Properties): oj.biz.AuthResponse;

            /**
             * Encodes the specified AuthResponse message. Does not implicitly {@link oj.biz.AuthResponse.verify|verify} messages.
             * @param message AuthResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AuthResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AuthResponse message, length delimited. Does not implicitly {@link oj.biz.AuthResponse.verify|verify} messages.
             * @param message AuthResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AuthResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AuthResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AuthResponse & oj.biz.AuthResponse.$Shape} AuthResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AuthResponse & oj.biz.AuthResponse.$Shape;

            /**
             * Decodes an AuthResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AuthResponse & oj.biz.AuthResponse.$Shape} AuthResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AuthResponse & oj.biz.AuthResponse.$Shape;

            /**
             * Verifies an AuthResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AuthResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AuthResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AuthResponse;

            /**
             * Creates a plain object from an AuthResponse message. Also converts values to other types if specified.
             * @param message AuthResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AuthResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AuthResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AuthResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AuthResponse {

            /** Properties of an AuthResponse. */
            interface $Properties {

                /** AuthResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** AuthResponse user */
                user?: (oj.common.User.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AuthResponse. */
            type $Shape = oj.biz.AuthResponse.$Properties;
        }

        /**
         * Properties of a VerificationCodeLoginRequest.
         * @deprecated Use oj.biz.VerificationCodeLoginRequest.$Properties instead.
         */
        interface IVerificationCodeLoginRequest extends oj.biz.VerificationCodeLoginRequest.$Properties {
        }

        /** Represents a VerificationCodeLoginRequest. */
        class VerificationCodeLoginRequest {

            /**
             * Constructs a new VerificationCodeLoginRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.VerificationCodeLoginRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** VerificationCodeLoginRequest email. */
            email: string;

            /** VerificationCodeLoginRequest verificationCode. */
            verificationCode: string;

            /**
             * Creates a new VerificationCodeLoginRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns VerificationCodeLoginRequest instance
             */
            static create(properties: oj.biz.VerificationCodeLoginRequest.$Shape): oj.biz.VerificationCodeLoginRequest & oj.biz.VerificationCodeLoginRequest.$Shape;
            static create(properties?: oj.biz.VerificationCodeLoginRequest.$Properties): oj.biz.VerificationCodeLoginRequest;

            /**
             * Encodes the specified VerificationCodeLoginRequest message. Does not implicitly {@link oj.biz.VerificationCodeLoginRequest.verify|verify} messages.
             * @param message VerificationCodeLoginRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.VerificationCodeLoginRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified VerificationCodeLoginRequest message, length delimited. Does not implicitly {@link oj.biz.VerificationCodeLoginRequest.verify|verify} messages.
             * @param message VerificationCodeLoginRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.VerificationCodeLoginRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a VerificationCodeLoginRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.VerificationCodeLoginRequest & oj.biz.VerificationCodeLoginRequest.$Shape} VerificationCodeLoginRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.VerificationCodeLoginRequest & oj.biz.VerificationCodeLoginRequest.$Shape;

            /**
             * Decodes a VerificationCodeLoginRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.VerificationCodeLoginRequest & oj.biz.VerificationCodeLoginRequest.$Shape} VerificationCodeLoginRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.VerificationCodeLoginRequest & oj.biz.VerificationCodeLoginRequest.$Shape;

            /**
             * Verifies a VerificationCodeLoginRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a VerificationCodeLoginRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns VerificationCodeLoginRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.VerificationCodeLoginRequest;

            /**
             * Creates a plain object from a VerificationCodeLoginRequest message. Also converts values to other types if specified.
             * @param message VerificationCodeLoginRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.VerificationCodeLoginRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this VerificationCodeLoginRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for VerificationCodeLoginRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace VerificationCodeLoginRequest {

            /** Properties of a VerificationCodeLoginRequest. */
            interface $Properties {

                /** VerificationCodeLoginRequest email */
                email?: (string|null);

                /** VerificationCodeLoginRequest verificationCode */
                verificationCode?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a VerificationCodeLoginRequest. */
            type $Shape = oj.biz.VerificationCodeLoginRequest.$Properties;
        }

        /**
         * Properties of a PasswordLoginRequest.
         * @deprecated Use oj.biz.PasswordLoginRequest.$Properties instead.
         */
        interface IPasswordLoginRequest extends oj.biz.PasswordLoginRequest.$Properties {
        }

        /** Represents a PasswordLoginRequest. */
        class PasswordLoginRequest {

            /**
             * Constructs a new PasswordLoginRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.PasswordLoginRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** PasswordLoginRequest emailOrName. */
            emailOrName: string;

            /** PasswordLoginRequest password. */
            password: string;

            /**
             * Creates a new PasswordLoginRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns PasswordLoginRequest instance
             */
            static create(properties: oj.biz.PasswordLoginRequest.$Shape): oj.biz.PasswordLoginRequest & oj.biz.PasswordLoginRequest.$Shape;
            static create(properties?: oj.biz.PasswordLoginRequest.$Properties): oj.biz.PasswordLoginRequest;

            /**
             * Encodes the specified PasswordLoginRequest message. Does not implicitly {@link oj.biz.PasswordLoginRequest.verify|verify} messages.
             * @param message PasswordLoginRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.PasswordLoginRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified PasswordLoginRequest message, length delimited. Does not implicitly {@link oj.biz.PasswordLoginRequest.verify|verify} messages.
             * @param message PasswordLoginRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.PasswordLoginRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a PasswordLoginRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.PasswordLoginRequest & oj.biz.PasswordLoginRequest.$Shape} PasswordLoginRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.PasswordLoginRequest & oj.biz.PasswordLoginRequest.$Shape;

            /**
             * Decodes a PasswordLoginRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.PasswordLoginRequest & oj.biz.PasswordLoginRequest.$Shape} PasswordLoginRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.PasswordLoginRequest & oj.biz.PasswordLoginRequest.$Shape;

            /**
             * Verifies a PasswordLoginRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a PasswordLoginRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns PasswordLoginRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.PasswordLoginRequest;

            /**
             * Creates a plain object from a PasswordLoginRequest message. Also converts values to other types if specified.
             * @param message PasswordLoginRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.PasswordLoginRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this PasswordLoginRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for PasswordLoginRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace PasswordLoginRequest {

            /** Properties of a PasswordLoginRequest. */
            interface $Properties {

                /** PasswordLoginRequest emailOrName */
                emailOrName?: (string|null);

                /** PasswordLoginRequest password */
                password?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a PasswordLoginRequest. */
            type $Shape = oj.biz.PasswordLoginRequest.$Properties;
        }

        /**
         * Properties of a SetPasswordRequest.
         * @deprecated Use oj.biz.SetPasswordRequest.$Properties instead.
         */
        interface ISetPasswordRequest extends oj.biz.SetPasswordRequest.$Properties {
        }

        /** Represents a SetPasswordRequest. */
        class SetPasswordRequest {

            /**
             * Constructs a new SetPasswordRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.SetPasswordRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** SetPasswordRequest password. */
            password: string;

            /**
             * Creates a new SetPasswordRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns SetPasswordRequest instance
             */
            static create(properties: oj.biz.SetPasswordRequest.$Shape): oj.biz.SetPasswordRequest & oj.biz.SetPasswordRequest.$Shape;
            static create(properties?: oj.biz.SetPasswordRequest.$Properties): oj.biz.SetPasswordRequest;

            /**
             * Encodes the specified SetPasswordRequest message. Does not implicitly {@link oj.biz.SetPasswordRequest.verify|verify} messages.
             * @param message SetPasswordRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.SetPasswordRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified SetPasswordRequest message, length delimited. Does not implicitly {@link oj.biz.SetPasswordRequest.verify|verify} messages.
             * @param message SetPasswordRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.SetPasswordRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a SetPasswordRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.SetPasswordRequest & oj.biz.SetPasswordRequest.$Shape} SetPasswordRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.SetPasswordRequest & oj.biz.SetPasswordRequest.$Shape;

            /**
             * Decodes a SetPasswordRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.SetPasswordRequest & oj.biz.SetPasswordRequest.$Shape} SetPasswordRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.SetPasswordRequest & oj.biz.SetPasswordRequest.$Shape;

            /**
             * Verifies a SetPasswordRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a SetPasswordRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns SetPasswordRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.SetPasswordRequest;

            /**
             * Creates a plain object from a SetPasswordRequest message. Also converts values to other types if specified.
             * @param message SetPasswordRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.SetPasswordRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this SetPasswordRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for SetPasswordRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace SetPasswordRequest {

            /** Properties of a SetPasswordRequest. */
            interface $Properties {

                /** SetPasswordRequest password */
                password?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a SetPasswordRequest. */
            type $Shape = oj.biz.SetPasswordRequest.$Properties;
        }

        /**
         * Properties of a ChangePasswordRequest.
         * @deprecated Use oj.biz.ChangePasswordRequest.$Properties instead.
         */
        interface IChangePasswordRequest extends oj.biz.ChangePasswordRequest.$Properties {
        }

        /** Represents a ChangePasswordRequest. */
        class ChangePasswordRequest {

            /**
             * Constructs a new ChangePasswordRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ChangePasswordRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ChangePasswordRequest verificationCode. */
            verificationCode: string;

            /** ChangePasswordRequest newPassword. */
            newPassword: string;

            /**
             * Creates a new ChangePasswordRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ChangePasswordRequest instance
             */
            static create(properties: oj.biz.ChangePasswordRequest.$Shape): oj.biz.ChangePasswordRequest & oj.biz.ChangePasswordRequest.$Shape;
            static create(properties?: oj.biz.ChangePasswordRequest.$Properties): oj.biz.ChangePasswordRequest;

            /**
             * Encodes the specified ChangePasswordRequest message. Does not implicitly {@link oj.biz.ChangePasswordRequest.verify|verify} messages.
             * @param message ChangePasswordRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ChangePasswordRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ChangePasswordRequest message, length delimited. Does not implicitly {@link oj.biz.ChangePasswordRequest.verify|verify} messages.
             * @param message ChangePasswordRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ChangePasswordRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ChangePasswordRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ChangePasswordRequest & oj.biz.ChangePasswordRequest.$Shape} ChangePasswordRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ChangePasswordRequest & oj.biz.ChangePasswordRequest.$Shape;

            /**
             * Decodes a ChangePasswordRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ChangePasswordRequest & oj.biz.ChangePasswordRequest.$Shape} ChangePasswordRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ChangePasswordRequest & oj.biz.ChangePasswordRequest.$Shape;

            /**
             * Verifies a ChangePasswordRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ChangePasswordRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ChangePasswordRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ChangePasswordRequest;

            /**
             * Creates a plain object from a ChangePasswordRequest message. Also converts values to other types if specified.
             * @param message ChangePasswordRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ChangePasswordRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ChangePasswordRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ChangePasswordRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ChangePasswordRequest {

            /** Properties of a ChangePasswordRequest. */
            interface $Properties {

                /** ChangePasswordRequest verificationCode */
                verificationCode?: (string|null);

                /** ChangePasswordRequest newPassword */
                newPassword?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ChangePasswordRequest. */
            type $Shape = oj.biz.ChangePasswordRequest.$Properties;
        }

        /**
         * Properties of a ChangeEmailRequest.
         * @deprecated Use oj.biz.ChangeEmailRequest.$Properties instead.
         */
        interface IChangeEmailRequest extends oj.biz.ChangeEmailRequest.$Properties {
        }

        /** Represents a ChangeEmailRequest. */
        class ChangeEmailRequest {

            /**
             * Constructs a new ChangeEmailRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ChangeEmailRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ChangeEmailRequest newEmail. */
            newEmail: string;

            /** ChangeEmailRequest verificationCode. */
            verificationCode: string;

            /**
             * Creates a new ChangeEmailRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ChangeEmailRequest instance
             */
            static create(properties: oj.biz.ChangeEmailRequest.$Shape): oj.biz.ChangeEmailRequest & oj.biz.ChangeEmailRequest.$Shape;
            static create(properties?: oj.biz.ChangeEmailRequest.$Properties): oj.biz.ChangeEmailRequest;

            /**
             * Encodes the specified ChangeEmailRequest message. Does not implicitly {@link oj.biz.ChangeEmailRequest.verify|verify} messages.
             * @param message ChangeEmailRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ChangeEmailRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ChangeEmailRequest message, length delimited. Does not implicitly {@link oj.biz.ChangeEmailRequest.verify|verify} messages.
             * @param message ChangeEmailRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ChangeEmailRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ChangeEmailRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ChangeEmailRequest & oj.biz.ChangeEmailRequest.$Shape} ChangeEmailRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ChangeEmailRequest & oj.biz.ChangeEmailRequest.$Shape;

            /**
             * Decodes a ChangeEmailRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ChangeEmailRequest & oj.biz.ChangeEmailRequest.$Shape} ChangeEmailRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ChangeEmailRequest & oj.biz.ChangeEmailRequest.$Shape;

            /**
             * Verifies a ChangeEmailRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ChangeEmailRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ChangeEmailRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ChangeEmailRequest;

            /**
             * Creates a plain object from a ChangeEmailRequest message. Also converts values to other types if specified.
             * @param message ChangeEmailRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ChangeEmailRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ChangeEmailRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ChangeEmailRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ChangeEmailRequest {

            /** Properties of a ChangeEmailRequest. */
            interface $Properties {

                /** ChangeEmailRequest newEmail */
                newEmail?: (string|null);

                /** ChangeEmailRequest verificationCode */
                verificationCode?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ChangeEmailRequest. */
            type $Shape = oj.biz.ChangeEmailRequest.$Properties;
        }

        /**
         * Properties of a DeleteAccountRequest.
         * @deprecated Use oj.biz.DeleteAccountRequest.$Properties instead.
         */
        interface IDeleteAccountRequest extends oj.biz.DeleteAccountRequest.$Properties {
        }

        /** Represents a DeleteAccountRequest. */
        class DeleteAccountRequest {

            /**
             * Constructs a new DeleteAccountRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.DeleteAccountRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** DeleteAccountRequest verificationCode. */
            verificationCode: string;

            /**
             * Creates a new DeleteAccountRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns DeleteAccountRequest instance
             */
            static create(properties: oj.biz.DeleteAccountRequest.$Shape): oj.biz.DeleteAccountRequest & oj.biz.DeleteAccountRequest.$Shape;
            static create(properties?: oj.biz.DeleteAccountRequest.$Properties): oj.biz.DeleteAccountRequest;

            /**
             * Encodes the specified DeleteAccountRequest message. Does not implicitly {@link oj.biz.DeleteAccountRequest.verify|verify} messages.
             * @param message DeleteAccountRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.DeleteAccountRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified DeleteAccountRequest message, length delimited. Does not implicitly {@link oj.biz.DeleteAccountRequest.verify|verify} messages.
             * @param message DeleteAccountRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.DeleteAccountRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a DeleteAccountRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.DeleteAccountRequest & oj.biz.DeleteAccountRequest.$Shape} DeleteAccountRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.DeleteAccountRequest & oj.biz.DeleteAccountRequest.$Shape;

            /**
             * Decodes a DeleteAccountRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.DeleteAccountRequest & oj.biz.DeleteAccountRequest.$Shape} DeleteAccountRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.DeleteAccountRequest & oj.biz.DeleteAccountRequest.$Shape;

            /**
             * Verifies a DeleteAccountRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a DeleteAccountRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns DeleteAccountRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.DeleteAccountRequest;

            /**
             * Creates a plain object from a DeleteAccountRequest message. Also converts values to other types if specified.
             * @param message DeleteAccountRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.DeleteAccountRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this DeleteAccountRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for DeleteAccountRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace DeleteAccountRequest {

            /** Properties of a DeleteAccountRequest. */
            interface $Properties {

                /** DeleteAccountRequest verificationCode */
                verificationCode?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a DeleteAccountRequest. */
            type $Shape = oj.biz.DeleteAccountRequest.$Properties;
        }

        /**
         * Properties of a GetUserProfileRequest.
         * @deprecated Use oj.biz.GetUserProfileRequest.$Properties instead.
         */
        interface IGetUserProfileRequest extends oj.biz.GetUserProfileRequest.$Properties {
        }

        /** Represents a GetUserProfileRequest. */
        class GetUserProfileRequest {

            /**
             * Constructs a new GetUserProfileRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetUserProfileRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetUserProfileRequest uid. */
            uid: number;

            /**
             * Creates a new GetUserProfileRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetUserProfileRequest instance
             */
            static create(properties: oj.biz.GetUserProfileRequest.$Shape): oj.biz.GetUserProfileRequest & oj.biz.GetUserProfileRequest.$Shape;
            static create(properties?: oj.biz.GetUserProfileRequest.$Properties): oj.biz.GetUserProfileRequest;

            /**
             * Encodes the specified GetUserProfileRequest message. Does not implicitly {@link oj.biz.GetUserProfileRequest.verify|verify} messages.
             * @param message GetUserProfileRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetUserProfileRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetUserProfileRequest message, length delimited. Does not implicitly {@link oj.biz.GetUserProfileRequest.verify|verify} messages.
             * @param message GetUserProfileRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetUserProfileRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetUserProfileRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetUserProfileRequest & oj.biz.GetUserProfileRequest.$Shape} GetUserProfileRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetUserProfileRequest & oj.biz.GetUserProfileRequest.$Shape;

            /**
             * Decodes a GetUserProfileRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetUserProfileRequest & oj.biz.GetUserProfileRequest.$Shape} GetUserProfileRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetUserProfileRequest & oj.biz.GetUserProfileRequest.$Shape;

            /**
             * Verifies a GetUserProfileRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetUserProfileRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetUserProfileRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetUserProfileRequest;

            /**
             * Creates a plain object from a GetUserProfileRequest message. Also converts values to other types if specified.
             * @param message GetUserProfileRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetUserProfileRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetUserProfileRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetUserProfileRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetUserProfileRequest {

            /** Properties of a GetUserProfileRequest. */
            interface $Properties {

                /** GetUserProfileRequest uid */
                uid?: (number|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetUserProfileRequest. */
            type $Shape = oj.biz.GetUserProfileRequest.$Properties;
        }

        /**
         * Properties of a GetUserProfileResponse.
         * @deprecated Use oj.biz.GetUserProfileResponse.$Properties instead.
         */
        interface IGetUserProfileResponse extends oj.biz.GetUserProfileResponse.$Properties {
        }

        /** Represents a GetUserProfileResponse. */
        class GetUserProfileResponse {

            /**
             * Constructs a new GetUserProfileResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetUserProfileResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetUserProfileResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetUserProfileResponse user. */
            user?: (oj.common.User.$Properties|null);

            /** GetUserProfileResponse statistics. */
            statistics?: (oj.common.UserStatistics.$Properties|null);

            /**
             * Creates a new GetUserProfileResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetUserProfileResponse instance
             */
            static create(properties: oj.biz.GetUserProfileResponse.$Shape): oj.biz.GetUserProfileResponse & oj.biz.GetUserProfileResponse.$Shape;
            static create(properties?: oj.biz.GetUserProfileResponse.$Properties): oj.biz.GetUserProfileResponse;

            /**
             * Encodes the specified GetUserProfileResponse message. Does not implicitly {@link oj.biz.GetUserProfileResponse.verify|verify} messages.
             * @param message GetUserProfileResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetUserProfileResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetUserProfileResponse message, length delimited. Does not implicitly {@link oj.biz.GetUserProfileResponse.verify|verify} messages.
             * @param message GetUserProfileResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetUserProfileResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetUserProfileResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetUserProfileResponse & oj.biz.GetUserProfileResponse.$Shape} GetUserProfileResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetUserProfileResponse & oj.biz.GetUserProfileResponse.$Shape;

            /**
             * Decodes a GetUserProfileResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetUserProfileResponse & oj.biz.GetUserProfileResponse.$Shape} GetUserProfileResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetUserProfileResponse & oj.biz.GetUserProfileResponse.$Shape;

            /**
             * Verifies a GetUserProfileResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetUserProfileResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetUserProfileResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetUserProfileResponse;

            /**
             * Creates a plain object from a GetUserProfileResponse message. Also converts values to other types if specified.
             * @param message GetUserProfileResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetUserProfileResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetUserProfileResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetUserProfileResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetUserProfileResponse {

            /** Properties of a GetUserProfileResponse. */
            interface $Properties {

                /** GetUserProfileResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetUserProfileResponse user */
                user?: (oj.common.User.$Properties|null);

                /** GetUserProfileResponse statistics */
                statistics?: (oj.common.UserStatistics.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetUserProfileResponse. */
            type $Shape = oj.biz.GetUserProfileResponse.$Properties;
        }

        /**
         * Properties of an UpdateProfileRequest.
         * @deprecated Use oj.biz.UpdateProfileRequest.$Properties instead.
         */
        interface IUpdateProfileRequest extends oj.biz.UpdateProfileRequest.$Properties {
        }

        /** Represents an UpdateProfileRequest. */
        class UpdateProfileRequest {

            /**
             * Constructs a new UpdateProfileRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.UpdateProfileRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** UpdateProfileRequest name. */
            name: string;

            /**
             * Creates a new UpdateProfileRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UpdateProfileRequest instance
             */
            static create(properties: oj.biz.UpdateProfileRequest.$Shape): oj.biz.UpdateProfileRequest & oj.biz.UpdateProfileRequest.$Shape;
            static create(properties?: oj.biz.UpdateProfileRequest.$Properties): oj.biz.UpdateProfileRequest;

            /**
             * Encodes the specified UpdateProfileRequest message. Does not implicitly {@link oj.biz.UpdateProfileRequest.verify|verify} messages.
             * @param message UpdateProfileRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.UpdateProfileRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UpdateProfileRequest message, length delimited. Does not implicitly {@link oj.biz.UpdateProfileRequest.verify|verify} messages.
             * @param message UpdateProfileRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.UpdateProfileRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an UpdateProfileRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.UpdateProfileRequest & oj.biz.UpdateProfileRequest.$Shape} UpdateProfileRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.UpdateProfileRequest & oj.biz.UpdateProfileRequest.$Shape;

            /**
             * Decodes an UpdateProfileRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.UpdateProfileRequest & oj.biz.UpdateProfileRequest.$Shape} UpdateProfileRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.UpdateProfileRequest & oj.biz.UpdateProfileRequest.$Shape;

            /**
             * Verifies an UpdateProfileRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an UpdateProfileRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UpdateProfileRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.UpdateProfileRequest;

            /**
             * Creates a plain object from an UpdateProfileRequest message. Also converts values to other types if specified.
             * @param message UpdateProfileRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.UpdateProfileRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UpdateProfileRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UpdateProfileRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UpdateProfileRequest {

            /** Properties of an UpdateProfileRequest. */
            interface $Properties {

                /** UpdateProfileRequest name */
                name?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an UpdateProfileRequest. */
            type $Shape = oj.biz.UpdateProfileRequest.$Properties;
        }

        /**
         * Properties of an UpdateProfileResponse.
         * @deprecated Use oj.biz.UpdateProfileResponse.$Properties instead.
         */
        interface IUpdateProfileResponse extends oj.biz.UpdateProfileResponse.$Properties {
        }

        /** Represents an UpdateProfileResponse. */
        class UpdateProfileResponse {

            /**
             * Constructs a new UpdateProfileResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.UpdateProfileResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** UpdateProfileResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** UpdateProfileResponse user. */
            user?: (oj.common.User.$Properties|null);

            /**
             * Creates a new UpdateProfileResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UpdateProfileResponse instance
             */
            static create(properties: oj.biz.UpdateProfileResponse.$Shape): oj.biz.UpdateProfileResponse & oj.biz.UpdateProfileResponse.$Shape;
            static create(properties?: oj.biz.UpdateProfileResponse.$Properties): oj.biz.UpdateProfileResponse;

            /**
             * Encodes the specified UpdateProfileResponse message. Does not implicitly {@link oj.biz.UpdateProfileResponse.verify|verify} messages.
             * @param message UpdateProfileResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.UpdateProfileResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UpdateProfileResponse message, length delimited. Does not implicitly {@link oj.biz.UpdateProfileResponse.verify|verify} messages.
             * @param message UpdateProfileResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.UpdateProfileResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an UpdateProfileResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.UpdateProfileResponse & oj.biz.UpdateProfileResponse.$Shape} UpdateProfileResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.UpdateProfileResponse & oj.biz.UpdateProfileResponse.$Shape;

            /**
             * Decodes an UpdateProfileResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.UpdateProfileResponse & oj.biz.UpdateProfileResponse.$Shape} UpdateProfileResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.UpdateProfileResponse & oj.biz.UpdateProfileResponse.$Shape;

            /**
             * Verifies an UpdateProfileResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an UpdateProfileResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UpdateProfileResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.UpdateProfileResponse;

            /**
             * Creates a plain object from an UpdateProfileResponse message. Also converts values to other types if specified.
             * @param message UpdateProfileResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.UpdateProfileResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UpdateProfileResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UpdateProfileResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UpdateProfileResponse {

            /** Properties of an UpdateProfileResponse. */
            interface $Properties {

                /** UpdateProfileResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** UpdateProfileResponse user */
                user?: (oj.common.User.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an UpdateProfileResponse. */
            type $Shape = oj.biz.UpdateProfileResponse.$Properties;
        }

        /**
         * Properties of an UpdateAvatarRequest.
         * @deprecated Use oj.biz.UpdateAvatarRequest.$Properties instead.
         */
        interface IUpdateAvatarRequest extends oj.biz.UpdateAvatarRequest.$Properties {
        }

        /** Represents an UpdateAvatarRequest. */
        class UpdateAvatarRequest {

            /**
             * Constructs a new UpdateAvatarRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.UpdateAvatarRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** UpdateAvatarRequest content. */
            content: Uint8Array;

            /** UpdateAvatarRequest contentType. */
            contentType: string;

            /**
             * Creates a new UpdateAvatarRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UpdateAvatarRequest instance
             */
            static create(properties: oj.biz.UpdateAvatarRequest.$Shape): oj.biz.UpdateAvatarRequest & oj.biz.UpdateAvatarRequest.$Shape;
            static create(properties?: oj.biz.UpdateAvatarRequest.$Properties): oj.biz.UpdateAvatarRequest;

            /**
             * Encodes the specified UpdateAvatarRequest message. Does not implicitly {@link oj.biz.UpdateAvatarRequest.verify|verify} messages.
             * @param message UpdateAvatarRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.UpdateAvatarRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UpdateAvatarRequest message, length delimited. Does not implicitly {@link oj.biz.UpdateAvatarRequest.verify|verify} messages.
             * @param message UpdateAvatarRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.UpdateAvatarRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an UpdateAvatarRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.UpdateAvatarRequest & oj.biz.UpdateAvatarRequest.$Shape} UpdateAvatarRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.UpdateAvatarRequest & oj.biz.UpdateAvatarRequest.$Shape;

            /**
             * Decodes an UpdateAvatarRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.UpdateAvatarRequest & oj.biz.UpdateAvatarRequest.$Shape} UpdateAvatarRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.UpdateAvatarRequest & oj.biz.UpdateAvatarRequest.$Shape;

            /**
             * Verifies an UpdateAvatarRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an UpdateAvatarRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UpdateAvatarRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.UpdateAvatarRequest;

            /**
             * Creates a plain object from an UpdateAvatarRequest message. Also converts values to other types if specified.
             * @param message UpdateAvatarRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.UpdateAvatarRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UpdateAvatarRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UpdateAvatarRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UpdateAvatarRequest {

            /** Properties of an UpdateAvatarRequest. */
            interface $Properties {

                /** UpdateAvatarRequest content */
                content?: (Uint8Array|null);

                /** UpdateAvatarRequest contentType */
                contentType?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an UpdateAvatarRequest. */
            type $Shape = oj.biz.UpdateAvatarRequest.$Properties;
        }

        /**
         * Properties of an UpdateAvatarResponse.
         * @deprecated Use oj.biz.UpdateAvatarResponse.$Properties instead.
         */
        interface IUpdateAvatarResponse extends oj.biz.UpdateAvatarResponse.$Properties {
        }

        /** Represents an UpdateAvatarResponse. */
        class UpdateAvatarResponse {

            /**
             * Constructs a new UpdateAvatarResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.UpdateAvatarResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** UpdateAvatarResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** UpdateAvatarResponse avatar. */
            avatar?: (oj.common.AvatarMetadata.$Properties|null);

            /**
             * Creates a new UpdateAvatarResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UpdateAvatarResponse instance
             */
            static create(properties: oj.biz.UpdateAvatarResponse.$Shape): oj.biz.UpdateAvatarResponse & oj.biz.UpdateAvatarResponse.$Shape;
            static create(properties?: oj.biz.UpdateAvatarResponse.$Properties): oj.biz.UpdateAvatarResponse;

            /**
             * Encodes the specified UpdateAvatarResponse message. Does not implicitly {@link oj.biz.UpdateAvatarResponse.verify|verify} messages.
             * @param message UpdateAvatarResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.UpdateAvatarResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UpdateAvatarResponse message, length delimited. Does not implicitly {@link oj.biz.UpdateAvatarResponse.verify|verify} messages.
             * @param message UpdateAvatarResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.UpdateAvatarResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an UpdateAvatarResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.UpdateAvatarResponse & oj.biz.UpdateAvatarResponse.$Shape} UpdateAvatarResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.UpdateAvatarResponse & oj.biz.UpdateAvatarResponse.$Shape;

            /**
             * Decodes an UpdateAvatarResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.UpdateAvatarResponse & oj.biz.UpdateAvatarResponse.$Shape} UpdateAvatarResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.UpdateAvatarResponse & oj.biz.UpdateAvatarResponse.$Shape;

            /**
             * Verifies an UpdateAvatarResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an UpdateAvatarResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UpdateAvatarResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.UpdateAvatarResponse;

            /**
             * Creates a plain object from an UpdateAvatarResponse message. Also converts values to other types if specified.
             * @param message UpdateAvatarResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.UpdateAvatarResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UpdateAvatarResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UpdateAvatarResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UpdateAvatarResponse {

            /** Properties of an UpdateAvatarResponse. */
            interface $Properties {

                /** UpdateAvatarResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** UpdateAvatarResponse avatar */
                avatar?: (oj.common.AvatarMetadata.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an UpdateAvatarResponse. */
            type $Shape = oj.biz.UpdateAvatarResponse.$Properties;
        }

        /**
         * Properties of a GetUserStatisticsResponse.
         * @deprecated Use oj.biz.GetUserStatisticsResponse.$Properties instead.
         */
        interface IGetUserStatisticsResponse extends oj.biz.GetUserStatisticsResponse.$Properties {
        }

        /** Represents a GetUserStatisticsResponse. */
        class GetUserStatisticsResponse {

            /**
             * Constructs a new GetUserStatisticsResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetUserStatisticsResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetUserStatisticsResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetUserStatisticsResponse statistics. */
            statistics?: (oj.common.UserStatistics.$Properties|null);

            /** GetUserStatisticsResponse recentSubmissions. */
            recentSubmissions: oj.common.Submission.$Properties[];

            /**
             * Creates a new GetUserStatisticsResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetUserStatisticsResponse instance
             */
            static create(properties: oj.biz.GetUserStatisticsResponse.$Shape): oj.biz.GetUserStatisticsResponse & oj.biz.GetUserStatisticsResponse.$Shape;
            static create(properties?: oj.biz.GetUserStatisticsResponse.$Properties): oj.biz.GetUserStatisticsResponse;

            /**
             * Encodes the specified GetUserStatisticsResponse message. Does not implicitly {@link oj.biz.GetUserStatisticsResponse.verify|verify} messages.
             * @param message GetUserStatisticsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetUserStatisticsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetUserStatisticsResponse message, length delimited. Does not implicitly {@link oj.biz.GetUserStatisticsResponse.verify|verify} messages.
             * @param message GetUserStatisticsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetUserStatisticsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetUserStatisticsResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetUserStatisticsResponse & oj.biz.GetUserStatisticsResponse.$Shape} GetUserStatisticsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetUserStatisticsResponse & oj.biz.GetUserStatisticsResponse.$Shape;

            /**
             * Decodes a GetUserStatisticsResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetUserStatisticsResponse & oj.biz.GetUserStatisticsResponse.$Shape} GetUserStatisticsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetUserStatisticsResponse & oj.biz.GetUserStatisticsResponse.$Shape;

            /**
             * Verifies a GetUserStatisticsResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetUserStatisticsResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetUserStatisticsResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetUserStatisticsResponse;

            /**
             * Creates a plain object from a GetUserStatisticsResponse message. Also converts values to other types if specified.
             * @param message GetUserStatisticsResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetUserStatisticsResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetUserStatisticsResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetUserStatisticsResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetUserStatisticsResponse {

            /** Properties of a GetUserStatisticsResponse. */
            interface $Properties {

                /** GetUserStatisticsResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetUserStatisticsResponse statistics */
                statistics?: (oj.common.UserStatistics.$Properties|null);

                /** GetUserStatisticsResponse recentSubmissions */
                recentSubmissions?: (oj.common.Submission.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetUserStatisticsResponse. */
            type $Shape = oj.biz.GetUserStatisticsResponse.$Properties;
        }

        /** Represents a UserService */
        class UserService extends $protobuf.rpc.Service {

            /**
             * Constructs a new UserService service.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             */
            constructor(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean);

            /**
             * Creates new UserService service using the specified rpc implementation.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             * @returns RPC service. Useful where requests and/or responses are streamed.
             */
            static create(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean): UserService;

            /** Calls SendRegistrationCode. */
            sendRegistrationCode: oj.biz.UserService.SendRegistrationCode;

            /** Calls Register. */
            register: oj.biz.UserService.Register;

            /** Calls LoginWithVerificationCode. */
            loginWithVerificationCode: oj.biz.UserService.LoginWithVerificationCode;

            /** Calls LoginWithPassword. */
            loginWithPassword: oj.biz.UserService.LoginWithPassword;

            /** Calls Logout. */
            logout: oj.biz.UserService.Logout;

            /** Calls SetPassword. */
            setPassword: oj.biz.UserService.SetPassword;

            /** Calls SendSecurityCode. */
            sendSecurityCode: oj.biz.UserService.SendSecurityCode;

            /** Calls ChangePassword. */
            changePassword: oj.biz.UserService.ChangePassword;

            /** Calls ChangeEmail. */
            changeEmail: oj.biz.UserService.ChangeEmail;

            /** Calls DeleteAccount. */
            deleteAccount: oj.biz.UserService.DeleteAccount;

            /** Calls GetCurrentUser. */
            getCurrentUser: oj.biz.UserService.GetCurrentUser;

            /** Calls GetUserProfile. */
            getUserProfile: oj.biz.UserService.GetUserProfile;

            /** Calls UpdateProfile. */
            updateProfile: oj.biz.UserService.UpdateProfile;

            /** Calls UpdateAvatar. */
            updateAvatar: oj.biz.UserService.UpdateAvatar;

            /** Calls DeleteAvatar. */
            deleteAvatar: oj.biz.UserService.DeleteAvatar;

            /** Calls GetStatistics. */
            getStatistics: oj.biz.UserService.GetStatistics;
        }

        namespace UserService {

            /**
             * Callback as used by {@link oj.biz.UserService#sendRegistrationCode}.
             * @param error Error, if any
             * @param [response] SendVerificationCodeResponse
             */
            type SendRegistrationCodeCallback = (error: (Error|null), response?: oj.biz.SendVerificationCodeResponse) => void;

            /** Calls SendRegistrationCode. */
            type SendRegistrationCode = {
              (request: oj.biz.ISendVerificationCodeRequest, callback: oj.biz.UserService.SendRegistrationCodeCallback): void;
              (request: oj.biz.ISendVerificationCodeRequest): Promise<oj.biz.SendVerificationCodeResponse>;
              readonly name: "SendRegistrationCode";
              readonly path: "/oj.biz.UserService/SendRegistrationCode";
              readonly requestType: "SendVerificationCodeRequest";
              readonly responseType: "SendVerificationCodeResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#register}.
             * @param error Error, if any
             * @param [response] AuthResponse
             */
            type RegisterCallback = (error: (Error|null), response?: oj.biz.AuthResponse) => void;

            /** Calls Register. */
            type Register = {
              (request: oj.biz.IRegisterRequest, callback: oj.biz.UserService.RegisterCallback): void;
              (request: oj.biz.IRegisterRequest): Promise<oj.biz.AuthResponse>;
              readonly name: "Register";
              readonly path: "/oj.biz.UserService/Register";
              readonly requestType: "RegisterRequest";
              readonly responseType: "AuthResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#loginWithVerificationCode}.
             * @param error Error, if any
             * @param [response] AuthResponse
             */
            type LoginWithVerificationCodeCallback = (error: (Error|null), response?: oj.biz.AuthResponse) => void;

            /** Calls LoginWithVerificationCode. */
            type LoginWithVerificationCode = {
              (request: oj.biz.IVerificationCodeLoginRequest, callback: oj.biz.UserService.LoginWithVerificationCodeCallback): void;
              (request: oj.biz.IVerificationCodeLoginRequest): Promise<oj.biz.AuthResponse>;
              readonly name: "LoginWithVerificationCode";
              readonly path: "/oj.biz.UserService/LoginWithVerificationCode";
              readonly requestType: "VerificationCodeLoginRequest";
              readonly responseType: "AuthResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#loginWithPassword}.
             * @param error Error, if any
             * @param [response] AuthResponse
             */
            type LoginWithPasswordCallback = (error: (Error|null), response?: oj.biz.AuthResponse) => void;

            /** Calls LoginWithPassword. */
            type LoginWithPassword = {
              (request: oj.biz.IPasswordLoginRequest, callback: oj.biz.UserService.LoginWithPasswordCallback): void;
              (request: oj.biz.IPasswordLoginRequest): Promise<oj.biz.AuthResponse>;
              readonly name: "LoginWithPassword";
              readonly path: "/oj.biz.UserService/LoginWithPassword";
              readonly requestType: "PasswordLoginRequest";
              readonly responseType: "AuthResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#logout}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type LogoutCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls Logout. */
            type Logout = {
              (request: oj.common.IEmptyRequest, callback: oj.biz.UserService.LogoutCallback): void;
              (request: oj.common.IEmptyRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "Logout";
              readonly path: "/oj.biz.UserService/Logout";
              readonly requestType: "oj.common.EmptyRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#setPassword}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type SetPasswordCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls SetPassword. */
            type SetPassword = {
              (request: oj.biz.ISetPasswordRequest, callback: oj.biz.UserService.SetPasswordCallback): void;
              (request: oj.biz.ISetPasswordRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "SetPassword";
              readonly path: "/oj.biz.UserService/SetPassword";
              readonly requestType: "SetPasswordRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#sendSecurityCode}.
             * @param error Error, if any
             * @param [response] SendVerificationCodeResponse
             */
            type SendSecurityCodeCallback = (error: (Error|null), response?: oj.biz.SendVerificationCodeResponse) => void;

            /** Calls SendSecurityCode. */
            type SendSecurityCode = {
              (request: oj.common.IEmptyRequest, callback: oj.biz.UserService.SendSecurityCodeCallback): void;
              (request: oj.common.IEmptyRequest): Promise<oj.biz.SendVerificationCodeResponse>;
              readonly name: "SendSecurityCode";
              readonly path: "/oj.biz.UserService/SendSecurityCode";
              readonly requestType: "oj.common.EmptyRequest";
              readonly responseType: "SendVerificationCodeResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#changePassword}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type ChangePasswordCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls ChangePassword. */
            type ChangePassword = {
              (request: oj.biz.IChangePasswordRequest, callback: oj.biz.UserService.ChangePasswordCallback): void;
              (request: oj.biz.IChangePasswordRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "ChangePassword";
              readonly path: "/oj.biz.UserService/ChangePassword";
              readonly requestType: "ChangePasswordRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#changeEmail}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type ChangeEmailCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls ChangeEmail. */
            type ChangeEmail = {
              (request: oj.biz.IChangeEmailRequest, callback: oj.biz.UserService.ChangeEmailCallback): void;
              (request: oj.biz.IChangeEmailRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "ChangeEmail";
              readonly path: "/oj.biz.UserService/ChangeEmail";
              readonly requestType: "ChangeEmailRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#deleteAccount}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type DeleteAccountCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls DeleteAccount. */
            type DeleteAccount = {
              (request: oj.biz.IDeleteAccountRequest, callback: oj.biz.UserService.DeleteAccountCallback): void;
              (request: oj.biz.IDeleteAccountRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "DeleteAccount";
              readonly path: "/oj.biz.UserService/DeleteAccount";
              readonly requestType: "DeleteAccountRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#getCurrentUser}.
             * @param error Error, if any
             * @param [response] GetUserProfileResponse
             */
            type GetCurrentUserCallback = (error: (Error|null), response?: oj.biz.GetUserProfileResponse) => void;

            /** Calls GetCurrentUser. */
            type GetCurrentUser = {
              (request: oj.common.IEmptyRequest, callback: oj.biz.UserService.GetCurrentUserCallback): void;
              (request: oj.common.IEmptyRequest): Promise<oj.biz.GetUserProfileResponse>;
              readonly name: "GetCurrentUser";
              readonly path: "/oj.biz.UserService/GetCurrentUser";
              readonly requestType: "oj.common.EmptyRequest";
              readonly responseType: "GetUserProfileResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#getUserProfile}.
             * @param error Error, if any
             * @param [response] GetUserProfileResponse
             */
            type GetUserProfileCallback = (error: (Error|null), response?: oj.biz.GetUserProfileResponse) => void;

            /** Calls GetUserProfile. */
            type GetUserProfile = {
              (request: oj.biz.IGetUserProfileRequest, callback: oj.biz.UserService.GetUserProfileCallback): void;
              (request: oj.biz.IGetUserProfileRequest): Promise<oj.biz.GetUserProfileResponse>;
              readonly name: "GetUserProfile";
              readonly path: "/oj.biz.UserService/GetUserProfile";
              readonly requestType: "GetUserProfileRequest";
              readonly responseType: "GetUserProfileResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#updateProfile}.
             * @param error Error, if any
             * @param [response] UpdateProfileResponse
             */
            type UpdateProfileCallback = (error: (Error|null), response?: oj.biz.UpdateProfileResponse) => void;

            /** Calls UpdateProfile. */
            type UpdateProfile = {
              (request: oj.biz.IUpdateProfileRequest, callback: oj.biz.UserService.UpdateProfileCallback): void;
              (request: oj.biz.IUpdateProfileRequest): Promise<oj.biz.UpdateProfileResponse>;
              readonly name: "UpdateProfile";
              readonly path: "/oj.biz.UserService/UpdateProfile";
              readonly requestType: "UpdateProfileRequest";
              readonly responseType: "UpdateProfileResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#updateAvatar}.
             * @param error Error, if any
             * @param [response] UpdateAvatarResponse
             */
            type UpdateAvatarCallback = (error: (Error|null), response?: oj.biz.UpdateAvatarResponse) => void;

            /** Calls UpdateAvatar. */
            type UpdateAvatar = {
              (request: oj.biz.IUpdateAvatarRequest, callback: oj.biz.UserService.UpdateAvatarCallback): void;
              (request: oj.biz.IUpdateAvatarRequest): Promise<oj.biz.UpdateAvatarResponse>;
              readonly name: "UpdateAvatar";
              readonly path: "/oj.biz.UserService/UpdateAvatar";
              readonly requestType: "UpdateAvatarRequest";
              readonly responseType: "UpdateAvatarResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#deleteAvatar}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type DeleteAvatarCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls DeleteAvatar. */
            type DeleteAvatar = {
              (request: oj.common.IEmptyRequest, callback: oj.biz.UserService.DeleteAvatarCallback): void;
              (request: oj.common.IEmptyRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "DeleteAvatar";
              readonly path: "/oj.biz.UserService/DeleteAvatar";
              readonly requestType: "oj.common.EmptyRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.UserService#getStatistics}.
             * @param error Error, if any
             * @param [response] GetUserStatisticsResponse
             */
            type GetStatisticsCallback = (error: (Error|null), response?: oj.biz.GetUserStatisticsResponse) => void;

            /** Calls GetStatistics. */
            type GetStatistics = {
              (request: oj.common.IEmptyRequest, callback: oj.biz.UserService.GetStatisticsCallback): void;
              (request: oj.common.IEmptyRequest): Promise<oj.biz.GetUserStatisticsResponse>;
              readonly name: "GetStatistics";
              readonly path: "/oj.biz.UserService/GetStatistics";
              readonly requestType: "oj.common.EmptyRequest";
              readonly responseType: "GetUserStatisticsResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };
        }

        /**
         * Properties of a GetQuestionListRequest.
         * @deprecated Use oj.biz.GetQuestionListRequest.$Properties instead.
         */
        interface IGetQuestionListRequest extends oj.biz.GetQuestionListRequest.$Properties {
        }

        /** Represents a GetQuestionListRequest. */
        class GetQuestionListRequest {

            /**
             * Constructs a new GetQuestionListRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetQuestionListRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetQuestionListRequest page. */
            page?: (oj.common.PageRequest.$Properties|null);

            /** GetQuestionListRequest search. */
            search: string;

            /** GetQuestionListRequest difficulty. */
            difficulty: string;

            /** GetQuestionListRequest includeHidden. */
            includeHidden: boolean;

            /**
             * Creates a new GetQuestionListRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetQuestionListRequest instance
             */
            static create(properties: oj.biz.GetQuestionListRequest.$Shape): oj.biz.GetQuestionListRequest & oj.biz.GetQuestionListRequest.$Shape;
            static create(properties?: oj.biz.GetQuestionListRequest.$Properties): oj.biz.GetQuestionListRequest;

            /**
             * Encodes the specified GetQuestionListRequest message. Does not implicitly {@link oj.biz.GetQuestionListRequest.verify|verify} messages.
             * @param message GetQuestionListRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetQuestionListRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetQuestionListRequest message, length delimited. Does not implicitly {@link oj.biz.GetQuestionListRequest.verify|verify} messages.
             * @param message GetQuestionListRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetQuestionListRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetQuestionListRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetQuestionListRequest & oj.biz.GetQuestionListRequest.$Shape} GetQuestionListRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetQuestionListRequest & oj.biz.GetQuestionListRequest.$Shape;

            /**
             * Decodes a GetQuestionListRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetQuestionListRequest & oj.biz.GetQuestionListRequest.$Shape} GetQuestionListRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetQuestionListRequest & oj.biz.GetQuestionListRequest.$Shape;

            /**
             * Verifies a GetQuestionListRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetQuestionListRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetQuestionListRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetQuestionListRequest;

            /**
             * Creates a plain object from a GetQuestionListRequest message. Also converts values to other types if specified.
             * @param message GetQuestionListRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetQuestionListRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetQuestionListRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetQuestionListRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetQuestionListRequest {

            /** Properties of a GetQuestionListRequest. */
            interface $Properties {

                /** GetQuestionListRequest page */
                page?: (oj.common.PageRequest.$Properties|null);

                /** GetQuestionListRequest search */
                search?: (string|null);

                /** GetQuestionListRequest difficulty */
                difficulty?: (string|null);

                /** GetQuestionListRequest includeHidden */
                includeHidden?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetQuestionListRequest. */
            type $Shape = oj.biz.GetQuestionListRequest.$Properties;
        }

        /**
         * Properties of a GetQuestionListResponse.
         * @deprecated Use oj.biz.GetQuestionListResponse.$Properties instead.
         */
        interface IGetQuestionListResponse extends oj.biz.GetQuestionListResponse.$Properties {
        }

        /** Represents a GetQuestionListResponse. */
        class GetQuestionListResponse {

            /**
             * Constructs a new GetQuestionListResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetQuestionListResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetQuestionListResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetQuestionListResponse page. */
            page?: (oj.common.PageResponse.$Properties|null);

            /** GetQuestionListResponse questions. */
            questions: oj.common.Question.$Properties[];

            /**
             * Creates a new GetQuestionListResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetQuestionListResponse instance
             */
            static create(properties: oj.biz.GetQuestionListResponse.$Shape): oj.biz.GetQuestionListResponse & oj.biz.GetQuestionListResponse.$Shape;
            static create(properties?: oj.biz.GetQuestionListResponse.$Properties): oj.biz.GetQuestionListResponse;

            /**
             * Encodes the specified GetQuestionListResponse message. Does not implicitly {@link oj.biz.GetQuestionListResponse.verify|verify} messages.
             * @param message GetQuestionListResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetQuestionListResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetQuestionListResponse message, length delimited. Does not implicitly {@link oj.biz.GetQuestionListResponse.verify|verify} messages.
             * @param message GetQuestionListResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetQuestionListResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetQuestionListResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetQuestionListResponse & oj.biz.GetQuestionListResponse.$Shape} GetQuestionListResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetQuestionListResponse & oj.biz.GetQuestionListResponse.$Shape;

            /**
             * Decodes a GetQuestionListResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetQuestionListResponse & oj.biz.GetQuestionListResponse.$Shape} GetQuestionListResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetQuestionListResponse & oj.biz.GetQuestionListResponse.$Shape;

            /**
             * Verifies a GetQuestionListResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetQuestionListResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetQuestionListResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetQuestionListResponse;

            /**
             * Creates a plain object from a GetQuestionListResponse message. Also converts values to other types if specified.
             * @param message GetQuestionListResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetQuestionListResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetQuestionListResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetQuestionListResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetQuestionListResponse {

            /** Properties of a GetQuestionListResponse. */
            interface $Properties {

                /** GetQuestionListResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetQuestionListResponse page */
                page?: (oj.common.PageResponse.$Properties|null);

                /** GetQuestionListResponse questions */
                questions?: (oj.common.Question.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetQuestionListResponse. */
            type $Shape = oj.biz.GetQuestionListResponse.$Properties;
        }

        /**
         * Properties of a QuestionIdRequest.
         * @deprecated Use oj.biz.QuestionIdRequest.$Properties instead.
         */
        interface IQuestionIdRequest extends oj.biz.QuestionIdRequest.$Properties {
        }

        /** Represents a QuestionIdRequest. */
        class QuestionIdRequest {

            /**
             * Constructs a new QuestionIdRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.QuestionIdRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** QuestionIdRequest questionId. */
            questionId: string;

            /**
             * Creates a new QuestionIdRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns QuestionIdRequest instance
             */
            static create(properties: oj.biz.QuestionIdRequest.$Shape): oj.biz.QuestionIdRequest & oj.biz.QuestionIdRequest.$Shape;
            static create(properties?: oj.biz.QuestionIdRequest.$Properties): oj.biz.QuestionIdRequest;

            /**
             * Encodes the specified QuestionIdRequest message. Does not implicitly {@link oj.biz.QuestionIdRequest.verify|verify} messages.
             * @param message QuestionIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.QuestionIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified QuestionIdRequest message, length delimited. Does not implicitly {@link oj.biz.QuestionIdRequest.verify|verify} messages.
             * @param message QuestionIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.QuestionIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a QuestionIdRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.QuestionIdRequest & oj.biz.QuestionIdRequest.$Shape} QuestionIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.QuestionIdRequest & oj.biz.QuestionIdRequest.$Shape;

            /**
             * Decodes a QuestionIdRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.QuestionIdRequest & oj.biz.QuestionIdRequest.$Shape} QuestionIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.QuestionIdRequest & oj.biz.QuestionIdRequest.$Shape;

            /**
             * Verifies a QuestionIdRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a QuestionIdRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns QuestionIdRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.QuestionIdRequest;

            /**
             * Creates a plain object from a QuestionIdRequest message. Also converts values to other types if specified.
             * @param message QuestionIdRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.QuestionIdRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this QuestionIdRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for QuestionIdRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace QuestionIdRequest {

            /** Properties of a QuestionIdRequest. */
            interface $Properties {

                /** QuestionIdRequest questionId */
                questionId?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a QuestionIdRequest. */
            type $Shape = oj.biz.QuestionIdRequest.$Properties;
        }

        /**
         * Properties of a GetQuestionDetailResponse.
         * @deprecated Use oj.biz.GetQuestionDetailResponse.$Properties instead.
         */
        interface IGetQuestionDetailResponse extends oj.biz.GetQuestionDetailResponse.$Properties {
        }

        /** Represents a GetQuestionDetailResponse. */
        class GetQuestionDetailResponse {

            /**
             * Constructs a new GetQuestionDetailResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetQuestionDetailResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetQuestionDetailResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetQuestionDetailResponse question. */
            question?: (oj.common.Question.$Properties|null);

            /** GetQuestionDetailResponse sampleTests. */
            sampleTests: oj.common.TestCase.$Properties[];

            /** GetQuestionDetailResponse passedByCurrentUser. */
            passedByCurrentUser: boolean;

            /**
             * Creates a new GetQuestionDetailResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetQuestionDetailResponse instance
             */
            static create(properties: oj.biz.GetQuestionDetailResponse.$Shape): oj.biz.GetQuestionDetailResponse & oj.biz.GetQuestionDetailResponse.$Shape;
            static create(properties?: oj.biz.GetQuestionDetailResponse.$Properties): oj.biz.GetQuestionDetailResponse;

            /**
             * Encodes the specified GetQuestionDetailResponse message. Does not implicitly {@link oj.biz.GetQuestionDetailResponse.verify|verify} messages.
             * @param message GetQuestionDetailResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetQuestionDetailResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetQuestionDetailResponse message, length delimited. Does not implicitly {@link oj.biz.GetQuestionDetailResponse.verify|verify} messages.
             * @param message GetQuestionDetailResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetQuestionDetailResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetQuestionDetailResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetQuestionDetailResponse & oj.biz.GetQuestionDetailResponse.$Shape} GetQuestionDetailResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetQuestionDetailResponse & oj.biz.GetQuestionDetailResponse.$Shape;

            /**
             * Decodes a GetQuestionDetailResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetQuestionDetailResponse & oj.biz.GetQuestionDetailResponse.$Shape} GetQuestionDetailResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetQuestionDetailResponse & oj.biz.GetQuestionDetailResponse.$Shape;

            /**
             * Verifies a GetQuestionDetailResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetQuestionDetailResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetQuestionDetailResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetQuestionDetailResponse;

            /**
             * Creates a plain object from a GetQuestionDetailResponse message. Also converts values to other types if specified.
             * @param message GetQuestionDetailResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetQuestionDetailResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetQuestionDetailResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetQuestionDetailResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetQuestionDetailResponse {

            /** Properties of a GetQuestionDetailResponse. */
            interface $Properties {

                /** GetQuestionDetailResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetQuestionDetailResponse question */
                question?: (oj.common.Question.$Properties|null);

                /** GetQuestionDetailResponse sampleTests */
                sampleTests?: (oj.common.TestCase.$Properties[]|null);

                /** GetQuestionDetailResponse passedByCurrentUser */
                passedByCurrentUser?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetQuestionDetailResponse. */
            type $Shape = oj.biz.GetQuestionDetailResponse.$Properties;
        }

        /**
         * Properties of a GetQuestionPassStatusResponse.
         * @deprecated Use oj.biz.GetQuestionPassStatusResponse.$Properties instead.
         */
        interface IGetQuestionPassStatusResponse extends oj.biz.GetQuestionPassStatusResponse.$Properties {
        }

        /** Represents a GetQuestionPassStatusResponse. */
        class GetQuestionPassStatusResponse {

            /**
             * Constructs a new GetQuestionPassStatusResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetQuestionPassStatusResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetQuestionPassStatusResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetQuestionPassStatusResponse passed. */
            passed: boolean;

            /**
             * Creates a new GetQuestionPassStatusResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetQuestionPassStatusResponse instance
             */
            static create(properties: oj.biz.GetQuestionPassStatusResponse.$Shape): oj.biz.GetQuestionPassStatusResponse & oj.biz.GetQuestionPassStatusResponse.$Shape;
            static create(properties?: oj.biz.GetQuestionPassStatusResponse.$Properties): oj.biz.GetQuestionPassStatusResponse;

            /**
             * Encodes the specified GetQuestionPassStatusResponse message. Does not implicitly {@link oj.biz.GetQuestionPassStatusResponse.verify|verify} messages.
             * @param message GetQuestionPassStatusResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetQuestionPassStatusResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetQuestionPassStatusResponse message, length delimited. Does not implicitly {@link oj.biz.GetQuestionPassStatusResponse.verify|verify} messages.
             * @param message GetQuestionPassStatusResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetQuestionPassStatusResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetQuestionPassStatusResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetQuestionPassStatusResponse & oj.biz.GetQuestionPassStatusResponse.$Shape} GetQuestionPassStatusResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetQuestionPassStatusResponse & oj.biz.GetQuestionPassStatusResponse.$Shape;

            /**
             * Decodes a GetQuestionPassStatusResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetQuestionPassStatusResponse & oj.biz.GetQuestionPassStatusResponse.$Shape} GetQuestionPassStatusResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetQuestionPassStatusResponse & oj.biz.GetQuestionPassStatusResponse.$Shape;

            /**
             * Verifies a GetQuestionPassStatusResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetQuestionPassStatusResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetQuestionPassStatusResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetQuestionPassStatusResponse;

            /**
             * Creates a plain object from a GetQuestionPassStatusResponse message. Also converts values to other types if specified.
             * @param message GetQuestionPassStatusResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetQuestionPassStatusResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetQuestionPassStatusResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetQuestionPassStatusResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetQuestionPassStatusResponse {

            /** Properties of a GetQuestionPassStatusResponse. */
            interface $Properties {

                /** GetQuestionPassStatusResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetQuestionPassStatusResponse passed */
                passed?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetQuestionPassStatusResponse. */
            type $Shape = oj.biz.GetQuestionPassStatusResponse.$Properties;
        }

        /**
         * Properties of a SaveQuestionRequest.
         * @deprecated Use oj.biz.SaveQuestionRequest.$Properties instead.
         */
        interface ISaveQuestionRequest extends oj.biz.SaveQuestionRequest.$Properties {
        }

        /** Represents a SaveQuestionRequest. */
        class SaveQuestionRequest {

            /**
             * Constructs a new SaveQuestionRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.SaveQuestionRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** SaveQuestionRequest question. */
            question?: (oj.common.Question.$Properties|null);

            /**
             * Creates a new SaveQuestionRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns SaveQuestionRequest instance
             */
            static create(properties: oj.biz.SaveQuestionRequest.$Shape): oj.biz.SaveQuestionRequest & oj.biz.SaveQuestionRequest.$Shape;
            static create(properties?: oj.biz.SaveQuestionRequest.$Properties): oj.biz.SaveQuestionRequest;

            /**
             * Encodes the specified SaveQuestionRequest message. Does not implicitly {@link oj.biz.SaveQuestionRequest.verify|verify} messages.
             * @param message SaveQuestionRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.SaveQuestionRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified SaveQuestionRequest message, length delimited. Does not implicitly {@link oj.biz.SaveQuestionRequest.verify|verify} messages.
             * @param message SaveQuestionRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.SaveQuestionRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a SaveQuestionRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.SaveQuestionRequest & oj.biz.SaveQuestionRequest.$Shape} SaveQuestionRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.SaveQuestionRequest & oj.biz.SaveQuestionRequest.$Shape;

            /**
             * Decodes a SaveQuestionRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.SaveQuestionRequest & oj.biz.SaveQuestionRequest.$Shape} SaveQuestionRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.SaveQuestionRequest & oj.biz.SaveQuestionRequest.$Shape;

            /**
             * Verifies a SaveQuestionRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a SaveQuestionRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns SaveQuestionRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.SaveQuestionRequest;

            /**
             * Creates a plain object from a SaveQuestionRequest message. Also converts values to other types if specified.
             * @param message SaveQuestionRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.SaveQuestionRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this SaveQuestionRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for SaveQuestionRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace SaveQuestionRequest {

            /** Properties of a SaveQuestionRequest. */
            interface $Properties {

                /** SaveQuestionRequest question */
                question?: (oj.common.Question.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a SaveQuestionRequest. */
            type $Shape = oj.biz.SaveQuestionRequest.$Properties;
        }

        /**
         * Properties of a QuestionResponse.
         * @deprecated Use oj.biz.QuestionResponse.$Properties instead.
         */
        interface IQuestionResponse extends oj.biz.QuestionResponse.$Properties {
        }

        /** Represents a QuestionResponse. */
        class QuestionResponse {

            /**
             * Constructs a new QuestionResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.QuestionResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** QuestionResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** QuestionResponse question. */
            question?: (oj.common.Question.$Properties|null);

            /**
             * Creates a new QuestionResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns QuestionResponse instance
             */
            static create(properties: oj.biz.QuestionResponse.$Shape): oj.biz.QuestionResponse & oj.biz.QuestionResponse.$Shape;
            static create(properties?: oj.biz.QuestionResponse.$Properties): oj.biz.QuestionResponse;

            /**
             * Encodes the specified QuestionResponse message. Does not implicitly {@link oj.biz.QuestionResponse.verify|verify} messages.
             * @param message QuestionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.QuestionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified QuestionResponse message, length delimited. Does not implicitly {@link oj.biz.QuestionResponse.verify|verify} messages.
             * @param message QuestionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.QuestionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a QuestionResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.QuestionResponse & oj.biz.QuestionResponse.$Shape} QuestionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.QuestionResponse & oj.biz.QuestionResponse.$Shape;

            /**
             * Decodes a QuestionResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.QuestionResponse & oj.biz.QuestionResponse.$Shape} QuestionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.QuestionResponse & oj.biz.QuestionResponse.$Shape;

            /**
             * Verifies a QuestionResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a QuestionResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns QuestionResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.QuestionResponse;

            /**
             * Creates a plain object from a QuestionResponse message. Also converts values to other types if specified.
             * @param message QuestionResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.QuestionResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this QuestionResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for QuestionResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace QuestionResponse {

            /** Properties of a QuestionResponse. */
            interface $Properties {

                /** QuestionResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** QuestionResponse question */
                question?: (oj.common.Question.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a QuestionResponse. */
            type $Shape = oj.biz.QuestionResponse.$Properties;
        }

        /**
         * Properties of a ListTestCasesRequest.
         * @deprecated Use oj.biz.ListTestCasesRequest.$Properties instead.
         */
        interface IListTestCasesRequest extends oj.biz.ListTestCasesRequest.$Properties {
        }

        /** Represents a ListTestCasesRequest. */
        class ListTestCasesRequest {

            /**
             * Constructs a new ListTestCasesRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListTestCasesRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListTestCasesRequest questionId. */
            questionId: string;

            /** ListTestCasesRequest sampleOnly. */
            sampleOnly: boolean;

            /**
             * Creates a new ListTestCasesRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListTestCasesRequest instance
             */
            static create(properties: oj.biz.ListTestCasesRequest.$Shape): oj.biz.ListTestCasesRequest & oj.biz.ListTestCasesRequest.$Shape;
            static create(properties?: oj.biz.ListTestCasesRequest.$Properties): oj.biz.ListTestCasesRequest;

            /**
             * Encodes the specified ListTestCasesRequest message. Does not implicitly {@link oj.biz.ListTestCasesRequest.verify|verify} messages.
             * @param message ListTestCasesRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListTestCasesRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListTestCasesRequest message, length delimited. Does not implicitly {@link oj.biz.ListTestCasesRequest.verify|verify} messages.
             * @param message ListTestCasesRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListTestCasesRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListTestCasesRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListTestCasesRequest & oj.biz.ListTestCasesRequest.$Shape} ListTestCasesRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListTestCasesRequest & oj.biz.ListTestCasesRequest.$Shape;

            /**
             * Decodes a ListTestCasesRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListTestCasesRequest & oj.biz.ListTestCasesRequest.$Shape} ListTestCasesRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListTestCasesRequest & oj.biz.ListTestCasesRequest.$Shape;

            /**
             * Verifies a ListTestCasesRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListTestCasesRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListTestCasesRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListTestCasesRequest;

            /**
             * Creates a plain object from a ListTestCasesRequest message. Also converts values to other types if specified.
             * @param message ListTestCasesRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListTestCasesRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListTestCasesRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListTestCasesRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListTestCasesRequest {

            /** Properties of a ListTestCasesRequest. */
            interface $Properties {

                /** ListTestCasesRequest questionId */
                questionId?: (string|null);

                /** ListTestCasesRequest sampleOnly */
                sampleOnly?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListTestCasesRequest. */
            type $Shape = oj.biz.ListTestCasesRequest.$Properties;
        }

        /**
         * Properties of a ListTestCasesResponse.
         * @deprecated Use oj.biz.ListTestCasesResponse.$Properties instead.
         */
        interface IListTestCasesResponse extends oj.biz.ListTestCasesResponse.$Properties {
        }

        /** Represents a ListTestCasesResponse. */
        class ListTestCasesResponse {

            /**
             * Constructs a new ListTestCasesResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListTestCasesResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListTestCasesResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** ListTestCasesResponse testCases. */
            testCases: oj.common.TestCase.$Properties[];

            /**
             * Creates a new ListTestCasesResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListTestCasesResponse instance
             */
            static create(properties: oj.biz.ListTestCasesResponse.$Shape): oj.biz.ListTestCasesResponse & oj.biz.ListTestCasesResponse.$Shape;
            static create(properties?: oj.biz.ListTestCasesResponse.$Properties): oj.biz.ListTestCasesResponse;

            /**
             * Encodes the specified ListTestCasesResponse message. Does not implicitly {@link oj.biz.ListTestCasesResponse.verify|verify} messages.
             * @param message ListTestCasesResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListTestCasesResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListTestCasesResponse message, length delimited. Does not implicitly {@link oj.biz.ListTestCasesResponse.verify|verify} messages.
             * @param message ListTestCasesResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListTestCasesResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListTestCasesResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListTestCasesResponse & oj.biz.ListTestCasesResponse.$Shape} ListTestCasesResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListTestCasesResponse & oj.biz.ListTestCasesResponse.$Shape;

            /**
             * Decodes a ListTestCasesResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListTestCasesResponse & oj.biz.ListTestCasesResponse.$Shape} ListTestCasesResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListTestCasesResponse & oj.biz.ListTestCasesResponse.$Shape;

            /**
             * Verifies a ListTestCasesResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListTestCasesResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListTestCasesResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListTestCasesResponse;

            /**
             * Creates a plain object from a ListTestCasesResponse message. Also converts values to other types if specified.
             * @param message ListTestCasesResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListTestCasesResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListTestCasesResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListTestCasesResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListTestCasesResponse {

            /** Properties of a ListTestCasesResponse. */
            interface $Properties {

                /** ListTestCasesResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** ListTestCasesResponse testCases */
                testCases?: (oj.common.TestCase.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListTestCasesResponse. */
            type $Shape = oj.biz.ListTestCasesResponse.$Properties;
        }

        /**
         * Properties of a CreateTestCaseRequest.
         * @deprecated Use oj.biz.CreateTestCaseRequest.$Properties instead.
         */
        interface ICreateTestCaseRequest extends oj.biz.CreateTestCaseRequest.$Properties {
        }

        /** Represents a CreateTestCaseRequest. */
        class CreateTestCaseRequest {

            /**
             * Constructs a new CreateTestCaseRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.CreateTestCaseRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CreateTestCaseRequest questionId. */
            questionId: string;

            /** CreateTestCaseRequest ordinal. */
            ordinal: number;

            /** CreateTestCaseRequest input. */
            input: string;

            /** CreateTestCaseRequest expectedOutput. */
            expectedOutput: string;

            /** CreateTestCaseRequest isSample. */
            isSample: boolean;

            /**
             * Creates a new CreateTestCaseRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CreateTestCaseRequest instance
             */
            static create(properties: oj.biz.CreateTestCaseRequest.$Shape): oj.biz.CreateTestCaseRequest & oj.biz.CreateTestCaseRequest.$Shape;
            static create(properties?: oj.biz.CreateTestCaseRequest.$Properties): oj.biz.CreateTestCaseRequest;

            /**
             * Encodes the specified CreateTestCaseRequest message. Does not implicitly {@link oj.biz.CreateTestCaseRequest.verify|verify} messages.
             * @param message CreateTestCaseRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.CreateTestCaseRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CreateTestCaseRequest message, length delimited. Does not implicitly {@link oj.biz.CreateTestCaseRequest.verify|verify} messages.
             * @param message CreateTestCaseRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.CreateTestCaseRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CreateTestCaseRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.CreateTestCaseRequest & oj.biz.CreateTestCaseRequest.$Shape} CreateTestCaseRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.CreateTestCaseRequest & oj.biz.CreateTestCaseRequest.$Shape;

            /**
             * Decodes a CreateTestCaseRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.CreateTestCaseRequest & oj.biz.CreateTestCaseRequest.$Shape} CreateTestCaseRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.CreateTestCaseRequest & oj.biz.CreateTestCaseRequest.$Shape;

            /**
             * Verifies a CreateTestCaseRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CreateTestCaseRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CreateTestCaseRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.CreateTestCaseRequest;

            /**
             * Creates a plain object from a CreateTestCaseRequest message. Also converts values to other types if specified.
             * @param message CreateTestCaseRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.CreateTestCaseRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CreateTestCaseRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CreateTestCaseRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CreateTestCaseRequest {

            /** Properties of a CreateTestCaseRequest. */
            interface $Properties {

                /** CreateTestCaseRequest questionId */
                questionId?: (string|null);

                /** CreateTestCaseRequest ordinal */
                ordinal?: (number|null);

                /** CreateTestCaseRequest input */
                input?: (string|null);

                /** CreateTestCaseRequest expectedOutput */
                expectedOutput?: (string|null);

                /** CreateTestCaseRequest isSample */
                isSample?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CreateTestCaseRequest. */
            type $Shape = oj.biz.CreateTestCaseRequest.$Properties;
        }

        /**
         * Properties of an UpdateTestCaseRequest.
         * @deprecated Use oj.biz.UpdateTestCaseRequest.$Properties instead.
         */
        interface IUpdateTestCaseRequest extends oj.biz.UpdateTestCaseRequest.$Properties {
        }

        /** Represents an UpdateTestCaseRequest. */
        class UpdateTestCaseRequest {

            /**
             * Constructs a new UpdateTestCaseRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.UpdateTestCaseRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** UpdateTestCaseRequest testCase. */
            testCase?: (oj.common.TestCase.$Properties|null);

            /**
             * Creates a new UpdateTestCaseRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UpdateTestCaseRequest instance
             */
            static create(properties: oj.biz.UpdateTestCaseRequest.$Shape): oj.biz.UpdateTestCaseRequest & oj.biz.UpdateTestCaseRequest.$Shape;
            static create(properties?: oj.biz.UpdateTestCaseRequest.$Properties): oj.biz.UpdateTestCaseRequest;

            /**
             * Encodes the specified UpdateTestCaseRequest message. Does not implicitly {@link oj.biz.UpdateTestCaseRequest.verify|verify} messages.
             * @param message UpdateTestCaseRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.UpdateTestCaseRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UpdateTestCaseRequest message, length delimited. Does not implicitly {@link oj.biz.UpdateTestCaseRequest.verify|verify} messages.
             * @param message UpdateTestCaseRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.UpdateTestCaseRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an UpdateTestCaseRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.UpdateTestCaseRequest & oj.biz.UpdateTestCaseRequest.$Shape} UpdateTestCaseRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.UpdateTestCaseRequest & oj.biz.UpdateTestCaseRequest.$Shape;

            /**
             * Decodes an UpdateTestCaseRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.UpdateTestCaseRequest & oj.biz.UpdateTestCaseRequest.$Shape} UpdateTestCaseRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.UpdateTestCaseRequest & oj.biz.UpdateTestCaseRequest.$Shape;

            /**
             * Verifies an UpdateTestCaseRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an UpdateTestCaseRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UpdateTestCaseRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.UpdateTestCaseRequest;

            /**
             * Creates a plain object from an UpdateTestCaseRequest message. Also converts values to other types if specified.
             * @param message UpdateTestCaseRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.UpdateTestCaseRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UpdateTestCaseRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UpdateTestCaseRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UpdateTestCaseRequest {

            /** Properties of an UpdateTestCaseRequest. */
            interface $Properties {

                /** UpdateTestCaseRequest testCase */
                testCase?: (oj.common.TestCase.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an UpdateTestCaseRequest. */
            type $Shape = oj.biz.UpdateTestCaseRequest.$Properties;
        }

        /**
         * Properties of a TestCaseIdRequest.
         * @deprecated Use oj.biz.TestCaseIdRequest.$Properties instead.
         */
        interface ITestCaseIdRequest extends oj.biz.TestCaseIdRequest.$Properties {
        }

        /** Represents a TestCaseIdRequest. */
        class TestCaseIdRequest {

            /**
             * Constructs a new TestCaseIdRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.TestCaseIdRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** TestCaseIdRequest testCaseId. */
            testCaseId: (Long|string);

            /**
             * Creates a new TestCaseIdRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns TestCaseIdRequest instance
             */
            static create(properties: oj.biz.TestCaseIdRequest.$Shape): oj.biz.TestCaseIdRequest & oj.biz.TestCaseIdRequest.$Shape;
            static create(properties?: oj.biz.TestCaseIdRequest.$Properties): oj.biz.TestCaseIdRequest;

            /**
             * Encodes the specified TestCaseIdRequest message. Does not implicitly {@link oj.biz.TestCaseIdRequest.verify|verify} messages.
             * @param message TestCaseIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.TestCaseIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified TestCaseIdRequest message, length delimited. Does not implicitly {@link oj.biz.TestCaseIdRequest.verify|verify} messages.
             * @param message TestCaseIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.TestCaseIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a TestCaseIdRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.TestCaseIdRequest & oj.biz.TestCaseIdRequest.$Shape} TestCaseIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.TestCaseIdRequest & oj.biz.TestCaseIdRequest.$Shape;

            /**
             * Decodes a TestCaseIdRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.TestCaseIdRequest & oj.biz.TestCaseIdRequest.$Shape} TestCaseIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.TestCaseIdRequest & oj.biz.TestCaseIdRequest.$Shape;

            /**
             * Verifies a TestCaseIdRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a TestCaseIdRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns TestCaseIdRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.TestCaseIdRequest;

            /**
             * Creates a plain object from a TestCaseIdRequest message. Also converts values to other types if specified.
             * @param message TestCaseIdRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.TestCaseIdRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this TestCaseIdRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for TestCaseIdRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace TestCaseIdRequest {

            /** Properties of a TestCaseIdRequest. */
            interface $Properties {

                /** TestCaseIdRequest testCaseId */
                testCaseId?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a TestCaseIdRequest. */
            type $Shape = oj.biz.TestCaseIdRequest.$Properties;
        }

        /**
         * Properties of a TestCaseResponse.
         * @deprecated Use oj.biz.TestCaseResponse.$Properties instead.
         */
        interface ITestCaseResponse extends oj.biz.TestCaseResponse.$Properties {
        }

        /** Represents a TestCaseResponse. */
        class TestCaseResponse {

            /**
             * Constructs a new TestCaseResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.TestCaseResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** TestCaseResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** TestCaseResponse testCase. */
            testCase?: (oj.common.TestCase.$Properties|null);

            /**
             * Creates a new TestCaseResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns TestCaseResponse instance
             */
            static create(properties: oj.biz.TestCaseResponse.$Shape): oj.biz.TestCaseResponse & oj.biz.TestCaseResponse.$Shape;
            static create(properties?: oj.biz.TestCaseResponse.$Properties): oj.biz.TestCaseResponse;

            /**
             * Encodes the specified TestCaseResponse message. Does not implicitly {@link oj.biz.TestCaseResponse.verify|verify} messages.
             * @param message TestCaseResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.TestCaseResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified TestCaseResponse message, length delimited. Does not implicitly {@link oj.biz.TestCaseResponse.verify|verify} messages.
             * @param message TestCaseResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.TestCaseResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a TestCaseResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.TestCaseResponse & oj.biz.TestCaseResponse.$Shape} TestCaseResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.TestCaseResponse & oj.biz.TestCaseResponse.$Shape;

            /**
             * Decodes a TestCaseResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.TestCaseResponse & oj.biz.TestCaseResponse.$Shape} TestCaseResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.TestCaseResponse & oj.biz.TestCaseResponse.$Shape;

            /**
             * Verifies a TestCaseResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a TestCaseResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns TestCaseResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.TestCaseResponse;

            /**
             * Creates a plain object from a TestCaseResponse message. Also converts values to other types if specified.
             * @param message TestCaseResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.TestCaseResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this TestCaseResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for TestCaseResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace TestCaseResponse {

            /** Properties of a TestCaseResponse. */
            interface $Properties {

                /** TestCaseResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** TestCaseResponse testCase */
                testCase?: (oj.common.TestCase.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a TestCaseResponse. */
            type $Shape = oj.biz.TestCaseResponse.$Properties;
        }

        /**
         * Properties of an InvalidateQuestionCacheRequest.
         * @deprecated Use oj.biz.InvalidateQuestionCacheRequest.$Properties instead.
         */
        interface IInvalidateQuestionCacheRequest extends oj.biz.InvalidateQuestionCacheRequest.$Properties {
        }

        /** Represents an InvalidateQuestionCacheRequest. */
        class InvalidateQuestionCacheRequest {

            /**
             * Constructs a new InvalidateQuestionCacheRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.InvalidateQuestionCacheRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** InvalidateQuestionCacheRequest questionId. */
            questionId: string;

            /**
             * Creates a new InvalidateQuestionCacheRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns InvalidateQuestionCacheRequest instance
             */
            static create(properties: oj.biz.InvalidateQuestionCacheRequest.$Shape): oj.biz.InvalidateQuestionCacheRequest & oj.biz.InvalidateQuestionCacheRequest.$Shape;
            static create(properties?: oj.biz.InvalidateQuestionCacheRequest.$Properties): oj.biz.InvalidateQuestionCacheRequest;

            /**
             * Encodes the specified InvalidateQuestionCacheRequest message. Does not implicitly {@link oj.biz.InvalidateQuestionCacheRequest.verify|verify} messages.
             * @param message InvalidateQuestionCacheRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.InvalidateQuestionCacheRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified InvalidateQuestionCacheRequest message, length delimited. Does not implicitly {@link oj.biz.InvalidateQuestionCacheRequest.verify|verify} messages.
             * @param message InvalidateQuestionCacheRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.InvalidateQuestionCacheRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an InvalidateQuestionCacheRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.InvalidateQuestionCacheRequest & oj.biz.InvalidateQuestionCacheRequest.$Shape} InvalidateQuestionCacheRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.InvalidateQuestionCacheRequest & oj.biz.InvalidateQuestionCacheRequest.$Shape;

            /**
             * Decodes an InvalidateQuestionCacheRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.InvalidateQuestionCacheRequest & oj.biz.InvalidateQuestionCacheRequest.$Shape} InvalidateQuestionCacheRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.InvalidateQuestionCacheRequest & oj.biz.InvalidateQuestionCacheRequest.$Shape;

            /**
             * Verifies an InvalidateQuestionCacheRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an InvalidateQuestionCacheRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns InvalidateQuestionCacheRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.InvalidateQuestionCacheRequest;

            /**
             * Creates a plain object from an InvalidateQuestionCacheRequest message. Also converts values to other types if specified.
             * @param message InvalidateQuestionCacheRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.InvalidateQuestionCacheRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this InvalidateQuestionCacheRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for InvalidateQuestionCacheRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace InvalidateQuestionCacheRequest {

            /** Properties of an InvalidateQuestionCacheRequest. */
            interface $Properties {

                /** InvalidateQuestionCacheRequest questionId */
                questionId?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an InvalidateQuestionCacheRequest. */
            type $Shape = oj.biz.InvalidateQuestionCacheRequest.$Properties;
        }

        /** Represents a QuestionService */
        class QuestionService extends $protobuf.rpc.Service {

            /**
             * Constructs a new QuestionService service.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             */
            constructor(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean);

            /**
             * Creates new QuestionService service using the specified rpc implementation.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             * @returns RPC service. Useful where requests and/or responses are streamed.
             */
            static create(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean): QuestionService;

            /** Calls ListQuestions. */
            listQuestions: oj.biz.QuestionService.ListQuestions;

            /** Calls GetQuestion. */
            getQuestion: oj.biz.QuestionService.GetQuestion;

            /** Calls GetPassStatus. */
            getPassStatus: oj.biz.QuestionService.GetPassStatus;

            /** Calls CreateQuestion. */
            createQuestion: oj.biz.QuestionService.CreateQuestion;

            /** Calls UpdateQuestion. */
            updateQuestion: oj.biz.QuestionService.UpdateQuestion;

            /** Calls DeleteQuestion. */
            deleteQuestion: oj.biz.QuestionService.DeleteQuestion;

            /** Calls ListTestCases. */
            listTestCases: oj.biz.QuestionService.ListTestCases;

            /** Calls CreateTestCase. */
            createTestCase: oj.biz.QuestionService.CreateTestCase;

            /** Calls UpdateTestCase. */
            updateTestCase: oj.biz.QuestionService.UpdateTestCase;

            /** Calls DeleteTestCase. */
            deleteTestCase: oj.biz.QuestionService.DeleteTestCase;

            /** Calls InvalidateCache. */
            invalidateCache: oj.biz.QuestionService.InvalidateCache;
        }

        namespace QuestionService {

            /**
             * Callback as used by {@link oj.biz.QuestionService#listQuestions}.
             * @param error Error, if any
             * @param [response] GetQuestionListResponse
             */
            type ListQuestionsCallback = (error: (Error|null), response?: oj.biz.GetQuestionListResponse) => void;

            /** Calls ListQuestions. */
            type ListQuestions = {
              (request: oj.biz.IGetQuestionListRequest, callback: oj.biz.QuestionService.ListQuestionsCallback): void;
              (request: oj.biz.IGetQuestionListRequest): Promise<oj.biz.GetQuestionListResponse>;
              readonly name: "ListQuestions";
              readonly path: "/oj.biz.QuestionService/ListQuestions";
              readonly requestType: "GetQuestionListRequest";
              readonly responseType: "GetQuestionListResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.QuestionService#getQuestion}.
             * @param error Error, if any
             * @param [response] GetQuestionDetailResponse
             */
            type GetQuestionCallback = (error: (Error|null), response?: oj.biz.GetQuestionDetailResponse) => void;

            /** Calls GetQuestion. */
            type GetQuestion = {
              (request: oj.biz.IQuestionIdRequest, callback: oj.biz.QuestionService.GetQuestionCallback): void;
              (request: oj.biz.IQuestionIdRequest): Promise<oj.biz.GetQuestionDetailResponse>;
              readonly name: "GetQuestion";
              readonly path: "/oj.biz.QuestionService/GetQuestion";
              readonly requestType: "QuestionIdRequest";
              readonly responseType: "GetQuestionDetailResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.QuestionService#getPassStatus}.
             * @param error Error, if any
             * @param [response] GetQuestionPassStatusResponse
             */
            type GetPassStatusCallback = (error: (Error|null), response?: oj.biz.GetQuestionPassStatusResponse) => void;

            /** Calls GetPassStatus. */
            type GetPassStatus = {
              (request: oj.biz.IQuestionIdRequest, callback: oj.biz.QuestionService.GetPassStatusCallback): void;
              (request: oj.biz.IQuestionIdRequest): Promise<oj.biz.GetQuestionPassStatusResponse>;
              readonly name: "GetPassStatus";
              readonly path: "/oj.biz.QuestionService/GetPassStatus";
              readonly requestType: "QuestionIdRequest";
              readonly responseType: "GetQuestionPassStatusResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.QuestionService#createQuestion}.
             * @param error Error, if any
             * @param [response] QuestionResponse
             */
            type CreateQuestionCallback = (error: (Error|null), response?: oj.biz.QuestionResponse) => void;

            /** Calls CreateQuestion. */
            type CreateQuestion = {
              (request: oj.biz.ISaveQuestionRequest, callback: oj.biz.QuestionService.CreateQuestionCallback): void;
              (request: oj.biz.ISaveQuestionRequest): Promise<oj.biz.QuestionResponse>;
              readonly name: "CreateQuestion";
              readonly path: "/oj.biz.QuestionService/CreateQuestion";
              readonly requestType: "SaveQuestionRequest";
              readonly responseType: "QuestionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.QuestionService#updateQuestion}.
             * @param error Error, if any
             * @param [response] QuestionResponse
             */
            type UpdateQuestionCallback = (error: (Error|null), response?: oj.biz.QuestionResponse) => void;

            /** Calls UpdateQuestion. */
            type UpdateQuestion = {
              (request: oj.biz.ISaveQuestionRequest, callback: oj.biz.QuestionService.UpdateQuestionCallback): void;
              (request: oj.biz.ISaveQuestionRequest): Promise<oj.biz.QuestionResponse>;
              readonly name: "UpdateQuestion";
              readonly path: "/oj.biz.QuestionService/UpdateQuestion";
              readonly requestType: "SaveQuestionRequest";
              readonly responseType: "QuestionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.QuestionService#deleteQuestion}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type DeleteQuestionCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls DeleteQuestion. */
            type DeleteQuestion = {
              (request: oj.biz.IQuestionIdRequest, callback: oj.biz.QuestionService.DeleteQuestionCallback): void;
              (request: oj.biz.IQuestionIdRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "DeleteQuestion";
              readonly path: "/oj.biz.QuestionService/DeleteQuestion";
              readonly requestType: "QuestionIdRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.QuestionService#listTestCases}.
             * @param error Error, if any
             * @param [response] ListTestCasesResponse
             */
            type ListTestCasesCallback = (error: (Error|null), response?: oj.biz.ListTestCasesResponse) => void;

            /** Calls ListTestCases. */
            type ListTestCases = {
              (request: oj.biz.IListTestCasesRequest, callback: oj.biz.QuestionService.ListTestCasesCallback): void;
              (request: oj.biz.IListTestCasesRequest): Promise<oj.biz.ListTestCasesResponse>;
              readonly name: "ListTestCases";
              readonly path: "/oj.biz.QuestionService/ListTestCases";
              readonly requestType: "ListTestCasesRequest";
              readonly responseType: "ListTestCasesResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.QuestionService#createTestCase}.
             * @param error Error, if any
             * @param [response] TestCaseResponse
             */
            type CreateTestCaseCallback = (error: (Error|null), response?: oj.biz.TestCaseResponse) => void;

            /** Calls CreateTestCase. */
            type CreateTestCase = {
              (request: oj.biz.ICreateTestCaseRequest, callback: oj.biz.QuestionService.CreateTestCaseCallback): void;
              (request: oj.biz.ICreateTestCaseRequest): Promise<oj.biz.TestCaseResponse>;
              readonly name: "CreateTestCase";
              readonly path: "/oj.biz.QuestionService/CreateTestCase";
              readonly requestType: "CreateTestCaseRequest";
              readonly responseType: "TestCaseResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.QuestionService#updateTestCase}.
             * @param error Error, if any
             * @param [response] TestCaseResponse
             */
            type UpdateTestCaseCallback = (error: (Error|null), response?: oj.biz.TestCaseResponse) => void;

            /** Calls UpdateTestCase. */
            type UpdateTestCase = {
              (request: oj.biz.IUpdateTestCaseRequest, callback: oj.biz.QuestionService.UpdateTestCaseCallback): void;
              (request: oj.biz.IUpdateTestCaseRequest): Promise<oj.biz.TestCaseResponse>;
              readonly name: "UpdateTestCase";
              readonly path: "/oj.biz.QuestionService/UpdateTestCase";
              readonly requestType: "UpdateTestCaseRequest";
              readonly responseType: "TestCaseResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.QuestionService#deleteTestCase}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type DeleteTestCaseCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls DeleteTestCase. */
            type DeleteTestCase = {
              (request: oj.biz.ITestCaseIdRequest, callback: oj.biz.QuestionService.DeleteTestCaseCallback): void;
              (request: oj.biz.ITestCaseIdRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "DeleteTestCase";
              readonly path: "/oj.biz.QuestionService/DeleteTestCase";
              readonly requestType: "TestCaseIdRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.QuestionService#invalidateCache}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type InvalidateCacheCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls InvalidateCache. */
            type InvalidateCache = {
              (request: oj.biz.IInvalidateQuestionCacheRequest, callback: oj.biz.QuestionService.InvalidateCacheCallback): void;
              (request: oj.biz.IInvalidateQuestionCacheRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "InvalidateCache";
              readonly path: "/oj.biz.QuestionService/InvalidateCache";
              readonly requestType: "InvalidateQuestionCacheRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };
        }

        /**
         * Properties of a ListSolutionsRequest.
         * @deprecated Use oj.biz.ListSolutionsRequest.$Properties instead.
         */
        interface IListSolutionsRequest extends oj.biz.ListSolutionsRequest.$Properties {
        }

        /** Represents a ListSolutionsRequest. */
        class ListSolutionsRequest {

            /**
             * Constructs a new ListSolutionsRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListSolutionsRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListSolutionsRequest page. */
            page?: (oj.common.PageRequest.$Properties|null);

            /** ListSolutionsRequest questionId. */
            questionId: string;

            /** ListSolutionsRequest sortBy. */
            sortBy: string;

            /** ListSolutionsRequest authorUid. */
            authorUid: number;

            /**
             * Creates a new ListSolutionsRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListSolutionsRequest instance
             */
            static create(properties: oj.biz.ListSolutionsRequest.$Shape): oj.biz.ListSolutionsRequest & oj.biz.ListSolutionsRequest.$Shape;
            static create(properties?: oj.biz.ListSolutionsRequest.$Properties): oj.biz.ListSolutionsRequest;

            /**
             * Encodes the specified ListSolutionsRequest message. Does not implicitly {@link oj.biz.ListSolutionsRequest.verify|verify} messages.
             * @param message ListSolutionsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListSolutionsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListSolutionsRequest message, length delimited. Does not implicitly {@link oj.biz.ListSolutionsRequest.verify|verify} messages.
             * @param message ListSolutionsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListSolutionsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListSolutionsRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListSolutionsRequest & oj.biz.ListSolutionsRequest.$Shape} ListSolutionsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListSolutionsRequest & oj.biz.ListSolutionsRequest.$Shape;

            /**
             * Decodes a ListSolutionsRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListSolutionsRequest & oj.biz.ListSolutionsRequest.$Shape} ListSolutionsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListSolutionsRequest & oj.biz.ListSolutionsRequest.$Shape;

            /**
             * Verifies a ListSolutionsRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListSolutionsRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListSolutionsRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListSolutionsRequest;

            /**
             * Creates a plain object from a ListSolutionsRequest message. Also converts values to other types if specified.
             * @param message ListSolutionsRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListSolutionsRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListSolutionsRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListSolutionsRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListSolutionsRequest {

            /** Properties of a ListSolutionsRequest. */
            interface $Properties {

                /** ListSolutionsRequest page */
                page?: (oj.common.PageRequest.$Properties|null);

                /** ListSolutionsRequest questionId */
                questionId?: (string|null);

                /** ListSolutionsRequest sortBy */
                sortBy?: (string|null);

                /** ListSolutionsRequest authorUid */
                authorUid?: (number|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListSolutionsRequest. */
            type $Shape = oj.biz.ListSolutionsRequest.$Properties;
        }

        /**
         * Properties of a ListSolutionsResponse.
         * @deprecated Use oj.biz.ListSolutionsResponse.$Properties instead.
         */
        interface IListSolutionsResponse extends oj.biz.ListSolutionsResponse.$Properties {
        }

        /** Represents a ListSolutionsResponse. */
        class ListSolutionsResponse {

            /**
             * Constructs a new ListSolutionsResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListSolutionsResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListSolutionsResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** ListSolutionsResponse page. */
            page?: (oj.common.PageResponse.$Properties|null);

            /** ListSolutionsResponse solutions. */
            solutions: oj.common.Solution.$Properties[];

            /**
             * Creates a new ListSolutionsResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListSolutionsResponse instance
             */
            static create(properties: oj.biz.ListSolutionsResponse.$Shape): oj.biz.ListSolutionsResponse & oj.biz.ListSolutionsResponse.$Shape;
            static create(properties?: oj.biz.ListSolutionsResponse.$Properties): oj.biz.ListSolutionsResponse;

            /**
             * Encodes the specified ListSolutionsResponse message. Does not implicitly {@link oj.biz.ListSolutionsResponse.verify|verify} messages.
             * @param message ListSolutionsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListSolutionsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListSolutionsResponse message, length delimited. Does not implicitly {@link oj.biz.ListSolutionsResponse.verify|verify} messages.
             * @param message ListSolutionsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListSolutionsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListSolutionsResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListSolutionsResponse & oj.biz.ListSolutionsResponse.$Shape} ListSolutionsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListSolutionsResponse & oj.biz.ListSolutionsResponse.$Shape;

            /**
             * Decodes a ListSolutionsResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListSolutionsResponse & oj.biz.ListSolutionsResponse.$Shape} ListSolutionsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListSolutionsResponse & oj.biz.ListSolutionsResponse.$Shape;

            /**
             * Verifies a ListSolutionsResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListSolutionsResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListSolutionsResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListSolutionsResponse;

            /**
             * Creates a plain object from a ListSolutionsResponse message. Also converts values to other types if specified.
             * @param message ListSolutionsResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListSolutionsResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListSolutionsResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListSolutionsResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListSolutionsResponse {

            /** Properties of a ListSolutionsResponse. */
            interface $Properties {

                /** ListSolutionsResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** ListSolutionsResponse page */
                page?: (oj.common.PageResponse.$Properties|null);

                /** ListSolutionsResponse solutions */
                solutions?: (oj.common.Solution.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListSolutionsResponse. */
            type $Shape = oj.biz.ListSolutionsResponse.$Properties;
        }

        /**
         * Properties of a SolutionIdRequest.
         * @deprecated Use oj.biz.SolutionIdRequest.$Properties instead.
         */
        interface ISolutionIdRequest extends oj.biz.SolutionIdRequest.$Properties {
        }

        /** Represents a SolutionIdRequest. */
        class SolutionIdRequest {

            /**
             * Constructs a new SolutionIdRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.SolutionIdRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** SolutionIdRequest solutionId. */
            solutionId: (Long|string);

            /**
             * Creates a new SolutionIdRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns SolutionIdRequest instance
             */
            static create(properties: oj.biz.SolutionIdRequest.$Shape): oj.biz.SolutionIdRequest & oj.biz.SolutionIdRequest.$Shape;
            static create(properties?: oj.biz.SolutionIdRequest.$Properties): oj.biz.SolutionIdRequest;

            /**
             * Encodes the specified SolutionIdRequest message. Does not implicitly {@link oj.biz.SolutionIdRequest.verify|verify} messages.
             * @param message SolutionIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.SolutionIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified SolutionIdRequest message, length delimited. Does not implicitly {@link oj.biz.SolutionIdRequest.verify|verify} messages.
             * @param message SolutionIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.SolutionIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a SolutionIdRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.SolutionIdRequest & oj.biz.SolutionIdRequest.$Shape} SolutionIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.SolutionIdRequest & oj.biz.SolutionIdRequest.$Shape;

            /**
             * Decodes a SolutionIdRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.SolutionIdRequest & oj.biz.SolutionIdRequest.$Shape} SolutionIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.SolutionIdRequest & oj.biz.SolutionIdRequest.$Shape;

            /**
             * Verifies a SolutionIdRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a SolutionIdRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns SolutionIdRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.SolutionIdRequest;

            /**
             * Creates a plain object from a SolutionIdRequest message. Also converts values to other types if specified.
             * @param message SolutionIdRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.SolutionIdRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this SolutionIdRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for SolutionIdRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace SolutionIdRequest {

            /** Properties of a SolutionIdRequest. */
            interface $Properties {

                /** SolutionIdRequest solutionId */
                solutionId?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a SolutionIdRequest. */
            type $Shape = oj.biz.SolutionIdRequest.$Properties;
        }

        /**
         * Properties of a GetSolutionResponse.
         * @deprecated Use oj.biz.GetSolutionResponse.$Properties instead.
         */
        interface IGetSolutionResponse extends oj.biz.GetSolutionResponse.$Properties {
        }

        /** Represents a GetSolutionResponse. */
        class GetSolutionResponse {

            /**
             * Constructs a new GetSolutionResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetSolutionResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetSolutionResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetSolutionResponse solution. */
            solution?: (oj.common.Solution.$Properties|null);

            /** GetSolutionResponse author. */
            author?: (oj.common.User.$Properties|null);

            /** GetSolutionResponse currentUserActions. */
            currentUserActions?: (oj.common.SolutionActionState.$Properties|null);

            /**
             * Creates a new GetSolutionResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetSolutionResponse instance
             */
            static create(properties: oj.biz.GetSolutionResponse.$Shape): oj.biz.GetSolutionResponse & oj.biz.GetSolutionResponse.$Shape;
            static create(properties?: oj.biz.GetSolutionResponse.$Properties): oj.biz.GetSolutionResponse;

            /**
             * Encodes the specified GetSolutionResponse message. Does not implicitly {@link oj.biz.GetSolutionResponse.verify|verify} messages.
             * @param message GetSolutionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetSolutionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetSolutionResponse message, length delimited. Does not implicitly {@link oj.biz.GetSolutionResponse.verify|verify} messages.
             * @param message GetSolutionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetSolutionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetSolutionResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetSolutionResponse & oj.biz.GetSolutionResponse.$Shape} GetSolutionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetSolutionResponse & oj.biz.GetSolutionResponse.$Shape;

            /**
             * Decodes a GetSolutionResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetSolutionResponse & oj.biz.GetSolutionResponse.$Shape} GetSolutionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetSolutionResponse & oj.biz.GetSolutionResponse.$Shape;

            /**
             * Verifies a GetSolutionResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetSolutionResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetSolutionResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetSolutionResponse;

            /**
             * Creates a plain object from a GetSolutionResponse message. Also converts values to other types if specified.
             * @param message GetSolutionResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetSolutionResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetSolutionResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetSolutionResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetSolutionResponse {

            /** Properties of a GetSolutionResponse. */
            interface $Properties {

                /** GetSolutionResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetSolutionResponse solution */
                solution?: (oj.common.Solution.$Properties|null);

                /** GetSolutionResponse author */
                author?: (oj.common.User.$Properties|null);

                /** GetSolutionResponse currentUserActions */
                currentUserActions?: (oj.common.SolutionActionState.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetSolutionResponse. */
            type $Shape = oj.biz.GetSolutionResponse.$Properties;
        }

        /**
         * Properties of a CreateSolutionRequest.
         * @deprecated Use oj.biz.CreateSolutionRequest.$Properties instead.
         */
        interface ICreateSolutionRequest extends oj.biz.CreateSolutionRequest.$Properties {
        }

        /** Represents a CreateSolutionRequest. */
        class CreateSolutionRequest {

            /**
             * Constructs a new CreateSolutionRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.CreateSolutionRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CreateSolutionRequest questionId. */
            questionId: string;

            /** CreateSolutionRequest title. */
            title: string;

            /** CreateSolutionRequest contentMd. */
            contentMd: string;

            /**
             * Creates a new CreateSolutionRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CreateSolutionRequest instance
             */
            static create(properties: oj.biz.CreateSolutionRequest.$Shape): oj.biz.CreateSolutionRequest & oj.biz.CreateSolutionRequest.$Shape;
            static create(properties?: oj.biz.CreateSolutionRequest.$Properties): oj.biz.CreateSolutionRequest;

            /**
             * Encodes the specified CreateSolutionRequest message. Does not implicitly {@link oj.biz.CreateSolutionRequest.verify|verify} messages.
             * @param message CreateSolutionRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.CreateSolutionRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CreateSolutionRequest message, length delimited. Does not implicitly {@link oj.biz.CreateSolutionRequest.verify|verify} messages.
             * @param message CreateSolutionRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.CreateSolutionRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CreateSolutionRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.CreateSolutionRequest & oj.biz.CreateSolutionRequest.$Shape} CreateSolutionRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.CreateSolutionRequest & oj.biz.CreateSolutionRequest.$Shape;

            /**
             * Decodes a CreateSolutionRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.CreateSolutionRequest & oj.biz.CreateSolutionRequest.$Shape} CreateSolutionRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.CreateSolutionRequest & oj.biz.CreateSolutionRequest.$Shape;

            /**
             * Verifies a CreateSolutionRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CreateSolutionRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CreateSolutionRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.CreateSolutionRequest;

            /**
             * Creates a plain object from a CreateSolutionRequest message. Also converts values to other types if specified.
             * @param message CreateSolutionRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.CreateSolutionRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CreateSolutionRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CreateSolutionRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CreateSolutionRequest {

            /** Properties of a CreateSolutionRequest. */
            interface $Properties {

                /** CreateSolutionRequest questionId */
                questionId?: (string|null);

                /** CreateSolutionRequest title */
                title?: (string|null);

                /** CreateSolutionRequest contentMd */
                contentMd?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CreateSolutionRequest. */
            type $Shape = oj.biz.CreateSolutionRequest.$Properties;
        }

        /**
         * Properties of an UpdateSolutionRequest.
         * @deprecated Use oj.biz.UpdateSolutionRequest.$Properties instead.
         */
        interface IUpdateSolutionRequest extends oj.biz.UpdateSolutionRequest.$Properties {
        }

        /** Represents an UpdateSolutionRequest. */
        class UpdateSolutionRequest {

            /**
             * Constructs a new UpdateSolutionRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.UpdateSolutionRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** UpdateSolutionRequest solutionId. */
            solutionId: (Long|string);

            /** UpdateSolutionRequest title. */
            title: string;

            /** UpdateSolutionRequest contentMd. */
            contentMd: string;

            /**
             * Creates a new UpdateSolutionRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UpdateSolutionRequest instance
             */
            static create(properties: oj.biz.UpdateSolutionRequest.$Shape): oj.biz.UpdateSolutionRequest & oj.biz.UpdateSolutionRequest.$Shape;
            static create(properties?: oj.biz.UpdateSolutionRequest.$Properties): oj.biz.UpdateSolutionRequest;

            /**
             * Encodes the specified UpdateSolutionRequest message. Does not implicitly {@link oj.biz.UpdateSolutionRequest.verify|verify} messages.
             * @param message UpdateSolutionRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.UpdateSolutionRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UpdateSolutionRequest message, length delimited. Does not implicitly {@link oj.biz.UpdateSolutionRequest.verify|verify} messages.
             * @param message UpdateSolutionRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.UpdateSolutionRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an UpdateSolutionRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.UpdateSolutionRequest & oj.biz.UpdateSolutionRequest.$Shape} UpdateSolutionRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.UpdateSolutionRequest & oj.biz.UpdateSolutionRequest.$Shape;

            /**
             * Decodes an UpdateSolutionRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.UpdateSolutionRequest & oj.biz.UpdateSolutionRequest.$Shape} UpdateSolutionRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.UpdateSolutionRequest & oj.biz.UpdateSolutionRequest.$Shape;

            /**
             * Verifies an UpdateSolutionRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an UpdateSolutionRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UpdateSolutionRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.UpdateSolutionRequest;

            /**
             * Creates a plain object from an UpdateSolutionRequest message. Also converts values to other types if specified.
             * @param message UpdateSolutionRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.UpdateSolutionRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UpdateSolutionRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UpdateSolutionRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UpdateSolutionRequest {

            /** Properties of an UpdateSolutionRequest. */
            interface $Properties {

                /** UpdateSolutionRequest solutionId */
                solutionId?: ((Long|string)|null);

                /** UpdateSolutionRequest title */
                title?: (string|null);

                /** UpdateSolutionRequest contentMd */
                contentMd?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an UpdateSolutionRequest. */
            type $Shape = oj.biz.UpdateSolutionRequest.$Properties;
        }

        /**
         * Properties of a SolutionResponse.
         * @deprecated Use oj.biz.SolutionResponse.$Properties instead.
         */
        interface ISolutionResponse extends oj.biz.SolutionResponse.$Properties {
        }

        /** Represents a SolutionResponse. */
        class SolutionResponse {

            /**
             * Constructs a new SolutionResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.SolutionResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** SolutionResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** SolutionResponse solution. */
            solution?: (oj.common.Solution.$Properties|null);

            /**
             * Creates a new SolutionResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns SolutionResponse instance
             */
            static create(properties: oj.biz.SolutionResponse.$Shape): oj.biz.SolutionResponse & oj.biz.SolutionResponse.$Shape;
            static create(properties?: oj.biz.SolutionResponse.$Properties): oj.biz.SolutionResponse;

            /**
             * Encodes the specified SolutionResponse message. Does not implicitly {@link oj.biz.SolutionResponse.verify|verify} messages.
             * @param message SolutionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.SolutionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified SolutionResponse message, length delimited. Does not implicitly {@link oj.biz.SolutionResponse.verify|verify} messages.
             * @param message SolutionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.SolutionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a SolutionResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.SolutionResponse & oj.biz.SolutionResponse.$Shape} SolutionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.SolutionResponse & oj.biz.SolutionResponse.$Shape;

            /**
             * Decodes a SolutionResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.SolutionResponse & oj.biz.SolutionResponse.$Shape} SolutionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.SolutionResponse & oj.biz.SolutionResponse.$Shape;

            /**
             * Verifies a SolutionResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a SolutionResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns SolutionResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.SolutionResponse;

            /**
             * Creates a plain object from a SolutionResponse message. Also converts values to other types if specified.
             * @param message SolutionResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.SolutionResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this SolutionResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for SolutionResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace SolutionResponse {

            /** Properties of a SolutionResponse. */
            interface $Properties {

                /** SolutionResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** SolutionResponse solution */
                solution?: (oj.common.Solution.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a SolutionResponse. */
            type $Shape = oj.biz.SolutionResponse.$Properties;
        }

        /**
         * Properties of a SolutionActionResponse.
         * @deprecated Use oj.biz.SolutionActionResponse.$Properties instead.
         */
        interface ISolutionActionResponse extends oj.biz.SolutionActionResponse.$Properties {
        }

        /** Represents a SolutionActionResponse. */
        class SolutionActionResponse {

            /**
             * Constructs a new SolutionActionResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.SolutionActionResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** SolutionActionResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** SolutionActionResponse actions. */
            actions?: (oj.common.SolutionActionState.$Properties|null);

            /** SolutionActionResponse likeCount. */
            likeCount: (Long|string);

            /** SolutionActionResponse favoriteCount. */
            favoriteCount: (Long|string);

            /**
             * Creates a new SolutionActionResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns SolutionActionResponse instance
             */
            static create(properties: oj.biz.SolutionActionResponse.$Shape): oj.biz.SolutionActionResponse & oj.biz.SolutionActionResponse.$Shape;
            static create(properties?: oj.biz.SolutionActionResponse.$Properties): oj.biz.SolutionActionResponse;

            /**
             * Encodes the specified SolutionActionResponse message. Does not implicitly {@link oj.biz.SolutionActionResponse.verify|verify} messages.
             * @param message SolutionActionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.SolutionActionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified SolutionActionResponse message, length delimited. Does not implicitly {@link oj.biz.SolutionActionResponse.verify|verify} messages.
             * @param message SolutionActionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.SolutionActionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a SolutionActionResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.SolutionActionResponse & oj.biz.SolutionActionResponse.$Shape} SolutionActionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.SolutionActionResponse & oj.biz.SolutionActionResponse.$Shape;

            /**
             * Decodes a SolutionActionResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.SolutionActionResponse & oj.biz.SolutionActionResponse.$Shape} SolutionActionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.SolutionActionResponse & oj.biz.SolutionActionResponse.$Shape;

            /**
             * Verifies a SolutionActionResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a SolutionActionResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns SolutionActionResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.SolutionActionResponse;

            /**
             * Creates a plain object from a SolutionActionResponse message. Also converts values to other types if specified.
             * @param message SolutionActionResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.SolutionActionResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this SolutionActionResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for SolutionActionResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace SolutionActionResponse {

            /** Properties of a SolutionActionResponse. */
            interface $Properties {

                /** SolutionActionResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** SolutionActionResponse actions */
                actions?: (oj.common.SolutionActionState.$Properties|null);

                /** SolutionActionResponse likeCount */
                likeCount?: ((Long|string)|null);

                /** SolutionActionResponse favoriteCount */
                favoriteCount?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a SolutionActionResponse. */
            type $Shape = oj.biz.SolutionActionResponse.$Properties;
        }

        /** Represents a SolutionService */
        class SolutionService extends $protobuf.rpc.Service {

            /**
             * Constructs a new SolutionService service.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             */
            constructor(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean);

            /**
             * Creates new SolutionService service using the specified rpc implementation.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             * @returns RPC service. Useful where requests and/or responses are streamed.
             */
            static create(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean): SolutionService;

            /** Calls ListSolutions. */
            listSolutions: oj.biz.SolutionService.ListSolutions;

            /** Calls GetSolution. */
            getSolution: oj.biz.SolutionService.GetSolution;

            /** Calls CreateSolution. */
            createSolution: oj.biz.SolutionService.CreateSolution;

            /** Calls UpdateSolution. */
            updateSolution: oj.biz.SolutionService.UpdateSolution;

            /** Calls DeleteSolution. */
            deleteSolution: oj.biz.SolutionService.DeleteSolution;

            /** Calls ToggleLike. */
            toggleLike: oj.biz.SolutionService.ToggleLike;

            /** Calls ToggleFavorite. */
            toggleFavorite: oj.biz.SolutionService.ToggleFavorite;

            /** Calls GetActionState. */
            getActionState: oj.biz.SolutionService.GetActionState;
        }

        namespace SolutionService {

            /**
             * Callback as used by {@link oj.biz.SolutionService#listSolutions}.
             * @param error Error, if any
             * @param [response] ListSolutionsResponse
             */
            type ListSolutionsCallback = (error: (Error|null), response?: oj.biz.ListSolutionsResponse) => void;

            /** Calls ListSolutions. */
            type ListSolutions = {
              (request: oj.biz.IListSolutionsRequest, callback: oj.biz.SolutionService.ListSolutionsCallback): void;
              (request: oj.biz.IListSolutionsRequest): Promise<oj.biz.ListSolutionsResponse>;
              readonly name: "ListSolutions";
              readonly path: "/oj.biz.SolutionService/ListSolutions";
              readonly requestType: "ListSolutionsRequest";
              readonly responseType: "ListSolutionsResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SolutionService#getSolution}.
             * @param error Error, if any
             * @param [response] GetSolutionResponse
             */
            type GetSolutionCallback = (error: (Error|null), response?: oj.biz.GetSolutionResponse) => void;

            /** Calls GetSolution. */
            type GetSolution = {
              (request: oj.biz.ISolutionIdRequest, callback: oj.biz.SolutionService.GetSolutionCallback): void;
              (request: oj.biz.ISolutionIdRequest): Promise<oj.biz.GetSolutionResponse>;
              readonly name: "GetSolution";
              readonly path: "/oj.biz.SolutionService/GetSolution";
              readonly requestType: "SolutionIdRequest";
              readonly responseType: "GetSolutionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SolutionService#createSolution}.
             * @param error Error, if any
             * @param [response] SolutionResponse
             */
            type CreateSolutionCallback = (error: (Error|null), response?: oj.biz.SolutionResponse) => void;

            /** Calls CreateSolution. */
            type CreateSolution = {
              (request: oj.biz.ICreateSolutionRequest, callback: oj.biz.SolutionService.CreateSolutionCallback): void;
              (request: oj.biz.ICreateSolutionRequest): Promise<oj.biz.SolutionResponse>;
              readonly name: "CreateSolution";
              readonly path: "/oj.biz.SolutionService/CreateSolution";
              readonly requestType: "CreateSolutionRequest";
              readonly responseType: "SolutionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SolutionService#updateSolution}.
             * @param error Error, if any
             * @param [response] SolutionResponse
             */
            type UpdateSolutionCallback = (error: (Error|null), response?: oj.biz.SolutionResponse) => void;

            /** Calls UpdateSolution. */
            type UpdateSolution = {
              (request: oj.biz.IUpdateSolutionRequest, callback: oj.biz.SolutionService.UpdateSolutionCallback): void;
              (request: oj.biz.IUpdateSolutionRequest): Promise<oj.biz.SolutionResponse>;
              readonly name: "UpdateSolution";
              readonly path: "/oj.biz.SolutionService/UpdateSolution";
              readonly requestType: "UpdateSolutionRequest";
              readonly responseType: "SolutionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SolutionService#deleteSolution}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type DeleteSolutionCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls DeleteSolution. */
            type DeleteSolution = {
              (request: oj.biz.ISolutionIdRequest, callback: oj.biz.SolutionService.DeleteSolutionCallback): void;
              (request: oj.biz.ISolutionIdRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "DeleteSolution";
              readonly path: "/oj.biz.SolutionService/DeleteSolution";
              readonly requestType: "SolutionIdRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SolutionService#toggleLike}.
             * @param error Error, if any
             * @param [response] SolutionActionResponse
             */
            type ToggleLikeCallback = (error: (Error|null), response?: oj.biz.SolutionActionResponse) => void;

            /** Calls ToggleLike. */
            type ToggleLike = {
              (request: oj.biz.ISolutionIdRequest, callback: oj.biz.SolutionService.ToggleLikeCallback): void;
              (request: oj.biz.ISolutionIdRequest): Promise<oj.biz.SolutionActionResponse>;
              readonly name: "ToggleLike";
              readonly path: "/oj.biz.SolutionService/ToggleLike";
              readonly requestType: "SolutionIdRequest";
              readonly responseType: "SolutionActionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SolutionService#toggleFavorite}.
             * @param error Error, if any
             * @param [response] SolutionActionResponse
             */
            type ToggleFavoriteCallback = (error: (Error|null), response?: oj.biz.SolutionActionResponse) => void;

            /** Calls ToggleFavorite. */
            type ToggleFavorite = {
              (request: oj.biz.ISolutionIdRequest, callback: oj.biz.SolutionService.ToggleFavoriteCallback): void;
              (request: oj.biz.ISolutionIdRequest): Promise<oj.biz.SolutionActionResponse>;
              readonly name: "ToggleFavorite";
              readonly path: "/oj.biz.SolutionService/ToggleFavorite";
              readonly requestType: "SolutionIdRequest";
              readonly responseType: "SolutionActionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SolutionService#getActionState}.
             * @param error Error, if any
             * @param [response] SolutionActionResponse
             */
            type GetActionStateCallback = (error: (Error|null), response?: oj.biz.SolutionActionResponse) => void;

            /** Calls GetActionState. */
            type GetActionState = {
              (request: oj.biz.ISolutionIdRequest, callback: oj.biz.SolutionService.GetActionStateCallback): void;
              (request: oj.biz.ISolutionIdRequest): Promise<oj.biz.SolutionActionResponse>;
              readonly name: "GetActionState";
              readonly path: "/oj.biz.SolutionService/GetActionState";
              readonly requestType: "SolutionIdRequest";
              readonly responseType: "SolutionActionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };
        }

        /**
         * Properties of a ListCommentsRequest.
         * @deprecated Use oj.biz.ListCommentsRequest.$Properties instead.
         */
        interface IListCommentsRequest extends oj.biz.ListCommentsRequest.$Properties {
        }

        /** Represents a ListCommentsRequest. */
        class ListCommentsRequest {

            /**
             * Constructs a new ListCommentsRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListCommentsRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListCommentsRequest solutionId. */
            solutionId: (Long|string);

            /** ListCommentsRequest page. */
            page?: (oj.common.PageRequest.$Properties|null);

            /**
             * Creates a new ListCommentsRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListCommentsRequest instance
             */
            static create(properties: oj.biz.ListCommentsRequest.$Shape): oj.biz.ListCommentsRequest & oj.biz.ListCommentsRequest.$Shape;
            static create(properties?: oj.biz.ListCommentsRequest.$Properties): oj.biz.ListCommentsRequest;

            /**
             * Encodes the specified ListCommentsRequest message. Does not implicitly {@link oj.biz.ListCommentsRequest.verify|verify} messages.
             * @param message ListCommentsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListCommentsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListCommentsRequest message, length delimited. Does not implicitly {@link oj.biz.ListCommentsRequest.verify|verify} messages.
             * @param message ListCommentsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListCommentsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListCommentsRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListCommentsRequest & oj.biz.ListCommentsRequest.$Shape} ListCommentsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListCommentsRequest & oj.biz.ListCommentsRequest.$Shape;

            /**
             * Decodes a ListCommentsRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListCommentsRequest & oj.biz.ListCommentsRequest.$Shape} ListCommentsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListCommentsRequest & oj.biz.ListCommentsRequest.$Shape;

            /**
             * Verifies a ListCommentsRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListCommentsRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListCommentsRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListCommentsRequest;

            /**
             * Creates a plain object from a ListCommentsRequest message. Also converts values to other types if specified.
             * @param message ListCommentsRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListCommentsRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListCommentsRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListCommentsRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListCommentsRequest {

            /** Properties of a ListCommentsRequest. */
            interface $Properties {

                /** ListCommentsRequest solutionId */
                solutionId?: ((Long|string)|null);

                /** ListCommentsRequest page */
                page?: (oj.common.PageRequest.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListCommentsRequest. */
            type $Shape = oj.biz.ListCommentsRequest.$Properties;
        }

        /**
         * Properties of a ListRepliesRequest.
         * @deprecated Use oj.biz.ListRepliesRequest.$Properties instead.
         */
        interface IListRepliesRequest extends oj.biz.ListRepliesRequest.$Properties {
        }

        /** Represents a ListRepliesRequest. */
        class ListRepliesRequest {

            /**
             * Constructs a new ListRepliesRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListRepliesRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListRepliesRequest parentCommentId. */
            parentCommentId: (Long|string);

            /** ListRepliesRequest page. */
            page?: (oj.common.PageRequest.$Properties|null);

            /**
             * Creates a new ListRepliesRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListRepliesRequest instance
             */
            static create(properties: oj.biz.ListRepliesRequest.$Shape): oj.biz.ListRepliesRequest & oj.biz.ListRepliesRequest.$Shape;
            static create(properties?: oj.biz.ListRepliesRequest.$Properties): oj.biz.ListRepliesRequest;

            /**
             * Encodes the specified ListRepliesRequest message. Does not implicitly {@link oj.biz.ListRepliesRequest.verify|verify} messages.
             * @param message ListRepliesRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListRepliesRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListRepliesRequest message, length delimited. Does not implicitly {@link oj.biz.ListRepliesRequest.verify|verify} messages.
             * @param message ListRepliesRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListRepliesRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListRepliesRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListRepliesRequest & oj.biz.ListRepliesRequest.$Shape} ListRepliesRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListRepliesRequest & oj.biz.ListRepliesRequest.$Shape;

            /**
             * Decodes a ListRepliesRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListRepliesRequest & oj.biz.ListRepliesRequest.$Shape} ListRepliesRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListRepliesRequest & oj.biz.ListRepliesRequest.$Shape;

            /**
             * Verifies a ListRepliesRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListRepliesRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListRepliesRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListRepliesRequest;

            /**
             * Creates a plain object from a ListRepliesRequest message. Also converts values to other types if specified.
             * @param message ListRepliesRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListRepliesRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListRepliesRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListRepliesRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListRepliesRequest {

            /** Properties of a ListRepliesRequest. */
            interface $Properties {

                /** ListRepliesRequest parentCommentId */
                parentCommentId?: ((Long|string)|null);

                /** ListRepliesRequest page */
                page?: (oj.common.PageRequest.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListRepliesRequest. */
            type $Shape = oj.biz.ListRepliesRequest.$Properties;
        }

        /**
         * Properties of a ListCommentsResponse.
         * @deprecated Use oj.biz.ListCommentsResponse.$Properties instead.
         */
        interface IListCommentsResponse extends oj.biz.ListCommentsResponse.$Properties {
        }

        /** Represents a ListCommentsResponse. */
        class ListCommentsResponse {

            /**
             * Constructs a new ListCommentsResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListCommentsResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListCommentsResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** ListCommentsResponse page. */
            page?: (oj.common.PageResponse.$Properties|null);

            /** ListCommentsResponse comments. */
            comments: oj.common.Comment.$Properties[];

            /** ListCommentsResponse currentUserActions. */
            currentUserActions: oj.common.CommentActionState.$Properties[];

            /**
             * Creates a new ListCommentsResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListCommentsResponse instance
             */
            static create(properties: oj.biz.ListCommentsResponse.$Shape): oj.biz.ListCommentsResponse & oj.biz.ListCommentsResponse.$Shape;
            static create(properties?: oj.biz.ListCommentsResponse.$Properties): oj.biz.ListCommentsResponse;

            /**
             * Encodes the specified ListCommentsResponse message. Does not implicitly {@link oj.biz.ListCommentsResponse.verify|verify} messages.
             * @param message ListCommentsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListCommentsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListCommentsResponse message, length delimited. Does not implicitly {@link oj.biz.ListCommentsResponse.verify|verify} messages.
             * @param message ListCommentsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListCommentsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListCommentsResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListCommentsResponse & oj.biz.ListCommentsResponse.$Shape} ListCommentsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListCommentsResponse & oj.biz.ListCommentsResponse.$Shape;

            /**
             * Decodes a ListCommentsResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListCommentsResponse & oj.biz.ListCommentsResponse.$Shape} ListCommentsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListCommentsResponse & oj.biz.ListCommentsResponse.$Shape;

            /**
             * Verifies a ListCommentsResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListCommentsResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListCommentsResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListCommentsResponse;

            /**
             * Creates a plain object from a ListCommentsResponse message. Also converts values to other types if specified.
             * @param message ListCommentsResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListCommentsResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListCommentsResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListCommentsResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListCommentsResponse {

            /** Properties of a ListCommentsResponse. */
            interface $Properties {

                /** ListCommentsResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** ListCommentsResponse page */
                page?: (oj.common.PageResponse.$Properties|null);

                /** ListCommentsResponse comments */
                comments?: (oj.common.Comment.$Properties[]|null);

                /** ListCommentsResponse currentUserActions */
                currentUserActions?: (oj.common.CommentActionState.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListCommentsResponse. */
            type $Shape = oj.biz.ListCommentsResponse.$Properties;
        }

        /**
         * Properties of a CreateCommentRequest.
         * @deprecated Use oj.biz.CreateCommentRequest.$Properties instead.
         */
        interface ICreateCommentRequest extends oj.biz.CreateCommentRequest.$Properties {
        }

        /** Represents a CreateCommentRequest. */
        class CreateCommentRequest {

            /**
             * Constructs a new CreateCommentRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.CreateCommentRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CreateCommentRequest solutionId. */
            solutionId: (Long|string);

            /** CreateCommentRequest parentCommentId. */
            parentCommentId: (Long|string);

            /** CreateCommentRequest content. */
            content: string;

            /**
             * Creates a new CreateCommentRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CreateCommentRequest instance
             */
            static create(properties: oj.biz.CreateCommentRequest.$Shape): oj.biz.CreateCommentRequest & oj.biz.CreateCommentRequest.$Shape;
            static create(properties?: oj.biz.CreateCommentRequest.$Properties): oj.biz.CreateCommentRequest;

            /**
             * Encodes the specified CreateCommentRequest message. Does not implicitly {@link oj.biz.CreateCommentRequest.verify|verify} messages.
             * @param message CreateCommentRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.CreateCommentRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CreateCommentRequest message, length delimited. Does not implicitly {@link oj.biz.CreateCommentRequest.verify|verify} messages.
             * @param message CreateCommentRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.CreateCommentRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CreateCommentRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.CreateCommentRequest & oj.biz.CreateCommentRequest.$Shape} CreateCommentRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.CreateCommentRequest & oj.biz.CreateCommentRequest.$Shape;

            /**
             * Decodes a CreateCommentRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.CreateCommentRequest & oj.biz.CreateCommentRequest.$Shape} CreateCommentRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.CreateCommentRequest & oj.biz.CreateCommentRequest.$Shape;

            /**
             * Verifies a CreateCommentRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CreateCommentRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CreateCommentRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.CreateCommentRequest;

            /**
             * Creates a plain object from a CreateCommentRequest message. Also converts values to other types if specified.
             * @param message CreateCommentRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.CreateCommentRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CreateCommentRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CreateCommentRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CreateCommentRequest {

            /** Properties of a CreateCommentRequest. */
            interface $Properties {

                /** CreateCommentRequest solutionId */
                solutionId?: ((Long|string)|null);

                /** CreateCommentRequest parentCommentId */
                parentCommentId?: ((Long|string)|null);

                /** CreateCommentRequest content */
                content?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CreateCommentRequest. */
            type $Shape = oj.biz.CreateCommentRequest.$Properties;
        }

        /**
         * Properties of an UpdateCommentRequest.
         * @deprecated Use oj.biz.UpdateCommentRequest.$Properties instead.
         */
        interface IUpdateCommentRequest extends oj.biz.UpdateCommentRequest.$Properties {
        }

        /** Represents an UpdateCommentRequest. */
        class UpdateCommentRequest {

            /**
             * Constructs a new UpdateCommentRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.UpdateCommentRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** UpdateCommentRequest commentId. */
            commentId: (Long|string);

            /** UpdateCommentRequest content. */
            content: string;

            /**
             * Creates a new UpdateCommentRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UpdateCommentRequest instance
             */
            static create(properties: oj.biz.UpdateCommentRequest.$Shape): oj.biz.UpdateCommentRequest & oj.biz.UpdateCommentRequest.$Shape;
            static create(properties?: oj.biz.UpdateCommentRequest.$Properties): oj.biz.UpdateCommentRequest;

            /**
             * Encodes the specified UpdateCommentRequest message. Does not implicitly {@link oj.biz.UpdateCommentRequest.verify|verify} messages.
             * @param message UpdateCommentRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.UpdateCommentRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UpdateCommentRequest message, length delimited. Does not implicitly {@link oj.biz.UpdateCommentRequest.verify|verify} messages.
             * @param message UpdateCommentRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.UpdateCommentRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an UpdateCommentRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.UpdateCommentRequest & oj.biz.UpdateCommentRequest.$Shape} UpdateCommentRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.UpdateCommentRequest & oj.biz.UpdateCommentRequest.$Shape;

            /**
             * Decodes an UpdateCommentRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.UpdateCommentRequest & oj.biz.UpdateCommentRequest.$Shape} UpdateCommentRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.UpdateCommentRequest & oj.biz.UpdateCommentRequest.$Shape;

            /**
             * Verifies an UpdateCommentRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an UpdateCommentRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UpdateCommentRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.UpdateCommentRequest;

            /**
             * Creates a plain object from an UpdateCommentRequest message. Also converts values to other types if specified.
             * @param message UpdateCommentRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.UpdateCommentRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UpdateCommentRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UpdateCommentRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UpdateCommentRequest {

            /** Properties of an UpdateCommentRequest. */
            interface $Properties {

                /** UpdateCommentRequest commentId */
                commentId?: ((Long|string)|null);

                /** UpdateCommentRequest content */
                content?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an UpdateCommentRequest. */
            type $Shape = oj.biz.UpdateCommentRequest.$Properties;
        }

        /**
         * Properties of a CommentIdRequest.
         * @deprecated Use oj.biz.CommentIdRequest.$Properties instead.
         */
        interface ICommentIdRequest extends oj.biz.CommentIdRequest.$Properties {
        }

        /** Represents a CommentIdRequest. */
        class CommentIdRequest {

            /**
             * Constructs a new CommentIdRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.CommentIdRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CommentIdRequest commentId. */
            commentId: (Long|string);

            /**
             * Creates a new CommentIdRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CommentIdRequest instance
             */
            static create(properties: oj.biz.CommentIdRequest.$Shape): oj.biz.CommentIdRequest & oj.biz.CommentIdRequest.$Shape;
            static create(properties?: oj.biz.CommentIdRequest.$Properties): oj.biz.CommentIdRequest;

            /**
             * Encodes the specified CommentIdRequest message. Does not implicitly {@link oj.biz.CommentIdRequest.verify|verify} messages.
             * @param message CommentIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.CommentIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CommentIdRequest message, length delimited. Does not implicitly {@link oj.biz.CommentIdRequest.verify|verify} messages.
             * @param message CommentIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.CommentIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CommentIdRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.CommentIdRequest & oj.biz.CommentIdRequest.$Shape} CommentIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.CommentIdRequest & oj.biz.CommentIdRequest.$Shape;

            /**
             * Decodes a CommentIdRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.CommentIdRequest & oj.biz.CommentIdRequest.$Shape} CommentIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.CommentIdRequest & oj.biz.CommentIdRequest.$Shape;

            /**
             * Verifies a CommentIdRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CommentIdRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CommentIdRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.CommentIdRequest;

            /**
             * Creates a plain object from a CommentIdRequest message. Also converts values to other types if specified.
             * @param message CommentIdRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.CommentIdRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CommentIdRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CommentIdRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CommentIdRequest {

            /** Properties of a CommentIdRequest. */
            interface $Properties {

                /** CommentIdRequest commentId */
                commentId?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CommentIdRequest. */
            type $Shape = oj.biz.CommentIdRequest.$Properties;
        }

        /**
         * Properties of a CommentResponse.
         * @deprecated Use oj.biz.CommentResponse.$Properties instead.
         */
        interface ICommentResponse extends oj.biz.CommentResponse.$Properties {
        }

        /** Represents a CommentResponse. */
        class CommentResponse {

            /**
             * Constructs a new CommentResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.CommentResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CommentResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** CommentResponse comment. */
            comment?: (oj.common.Comment.$Properties|null);

            /**
             * Creates a new CommentResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CommentResponse instance
             */
            static create(properties: oj.biz.CommentResponse.$Shape): oj.biz.CommentResponse & oj.biz.CommentResponse.$Shape;
            static create(properties?: oj.biz.CommentResponse.$Properties): oj.biz.CommentResponse;

            /**
             * Encodes the specified CommentResponse message. Does not implicitly {@link oj.biz.CommentResponse.verify|verify} messages.
             * @param message CommentResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.CommentResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CommentResponse message, length delimited. Does not implicitly {@link oj.biz.CommentResponse.verify|verify} messages.
             * @param message CommentResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.CommentResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CommentResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.CommentResponse & oj.biz.CommentResponse.$Shape} CommentResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.CommentResponse & oj.biz.CommentResponse.$Shape;

            /**
             * Decodes a CommentResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.CommentResponse & oj.biz.CommentResponse.$Shape} CommentResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.CommentResponse & oj.biz.CommentResponse.$Shape;

            /**
             * Verifies a CommentResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CommentResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CommentResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.CommentResponse;

            /**
             * Creates a plain object from a CommentResponse message. Also converts values to other types if specified.
             * @param message CommentResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.CommentResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CommentResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CommentResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CommentResponse {

            /** Properties of a CommentResponse. */
            interface $Properties {

                /** CommentResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** CommentResponse comment */
                comment?: (oj.common.Comment.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CommentResponse. */
            type $Shape = oj.biz.CommentResponse.$Properties;
        }

        /**
         * Properties of a CommentActionResponse.
         * @deprecated Use oj.biz.CommentActionResponse.$Properties instead.
         */
        interface ICommentActionResponse extends oj.biz.CommentActionResponse.$Properties {
        }

        /** Represents a CommentActionResponse. */
        class CommentActionResponse {

            /**
             * Constructs a new CommentActionResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.CommentActionResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CommentActionResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** CommentActionResponse actions. */
            actions?: (oj.common.CommentActionState.$Properties|null);

            /** CommentActionResponse likeCount. */
            likeCount: (Long|string);

            /** CommentActionResponse favoriteCount. */
            favoriteCount: (Long|string);

            /**
             * Creates a new CommentActionResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CommentActionResponse instance
             */
            static create(properties: oj.biz.CommentActionResponse.$Shape): oj.biz.CommentActionResponse & oj.biz.CommentActionResponse.$Shape;
            static create(properties?: oj.biz.CommentActionResponse.$Properties): oj.biz.CommentActionResponse;

            /**
             * Encodes the specified CommentActionResponse message. Does not implicitly {@link oj.biz.CommentActionResponse.verify|verify} messages.
             * @param message CommentActionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.CommentActionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CommentActionResponse message, length delimited. Does not implicitly {@link oj.biz.CommentActionResponse.verify|verify} messages.
             * @param message CommentActionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.CommentActionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CommentActionResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.CommentActionResponse & oj.biz.CommentActionResponse.$Shape} CommentActionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.CommentActionResponse & oj.biz.CommentActionResponse.$Shape;

            /**
             * Decodes a CommentActionResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.CommentActionResponse & oj.biz.CommentActionResponse.$Shape} CommentActionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.CommentActionResponse & oj.biz.CommentActionResponse.$Shape;

            /**
             * Verifies a CommentActionResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CommentActionResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CommentActionResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.CommentActionResponse;

            /**
             * Creates a plain object from a CommentActionResponse message. Also converts values to other types if specified.
             * @param message CommentActionResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.CommentActionResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CommentActionResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CommentActionResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CommentActionResponse {

            /** Properties of a CommentActionResponse. */
            interface $Properties {

                /** CommentActionResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** CommentActionResponse actions */
                actions?: (oj.common.CommentActionState.$Properties|null);

                /** CommentActionResponse likeCount */
                likeCount?: ((Long|string)|null);

                /** CommentActionResponse favoriteCount */
                favoriteCount?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CommentActionResponse. */
            type $Shape = oj.biz.CommentActionResponse.$Properties;
        }

        /**
         * Properties of a GetCommentActionsRequest.
         * @deprecated Use oj.biz.GetCommentActionsRequest.$Properties instead.
         */
        interface IGetCommentActionsRequest extends oj.biz.GetCommentActionsRequest.$Properties {
        }

        /** Represents a GetCommentActionsRequest. */
        class GetCommentActionsRequest {

            /**
             * Constructs a new GetCommentActionsRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetCommentActionsRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetCommentActionsRequest commentIds. */
            commentIds: ((Long|string)|string)[];

            /**
             * Creates a new GetCommentActionsRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetCommentActionsRequest instance
             */
            static create(properties: oj.biz.GetCommentActionsRequest.$Shape): oj.biz.GetCommentActionsRequest & oj.biz.GetCommentActionsRequest.$Shape;
            static create(properties?: oj.biz.GetCommentActionsRequest.$Properties): oj.biz.GetCommentActionsRequest;

            /**
             * Encodes the specified GetCommentActionsRequest message. Does not implicitly {@link oj.biz.GetCommentActionsRequest.verify|verify} messages.
             * @param message GetCommentActionsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetCommentActionsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetCommentActionsRequest message, length delimited. Does not implicitly {@link oj.biz.GetCommentActionsRequest.verify|verify} messages.
             * @param message GetCommentActionsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetCommentActionsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetCommentActionsRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetCommentActionsRequest & oj.biz.GetCommentActionsRequest.$Shape} GetCommentActionsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetCommentActionsRequest & oj.biz.GetCommentActionsRequest.$Shape;

            /**
             * Decodes a GetCommentActionsRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetCommentActionsRequest & oj.biz.GetCommentActionsRequest.$Shape} GetCommentActionsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetCommentActionsRequest & oj.biz.GetCommentActionsRequest.$Shape;

            /**
             * Verifies a GetCommentActionsRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetCommentActionsRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetCommentActionsRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetCommentActionsRequest;

            /**
             * Creates a plain object from a GetCommentActionsRequest message. Also converts values to other types if specified.
             * @param message GetCommentActionsRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetCommentActionsRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetCommentActionsRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetCommentActionsRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetCommentActionsRequest {

            /** Properties of a GetCommentActionsRequest. */
            interface $Properties {

                /** GetCommentActionsRequest commentIds */
                commentIds?: (((Long|string)|string)[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetCommentActionsRequest. */
            type $Shape = oj.biz.GetCommentActionsRequest.$Properties;
        }

        /**
         * Properties of a GetCommentActionsResponse.
         * @deprecated Use oj.biz.GetCommentActionsResponse.$Properties instead.
         */
        interface IGetCommentActionsResponse extends oj.biz.GetCommentActionsResponse.$Properties {
        }

        /** Represents a GetCommentActionsResponse. */
        class GetCommentActionsResponse {

            /**
             * Constructs a new GetCommentActionsResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetCommentActionsResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetCommentActionsResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetCommentActionsResponse actions. */
            actions: oj.common.CommentActionState.$Properties[];

            /**
             * Creates a new GetCommentActionsResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetCommentActionsResponse instance
             */
            static create(properties: oj.biz.GetCommentActionsResponse.$Shape): oj.biz.GetCommentActionsResponse & oj.biz.GetCommentActionsResponse.$Shape;
            static create(properties?: oj.biz.GetCommentActionsResponse.$Properties): oj.biz.GetCommentActionsResponse;

            /**
             * Encodes the specified GetCommentActionsResponse message. Does not implicitly {@link oj.biz.GetCommentActionsResponse.verify|verify} messages.
             * @param message GetCommentActionsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetCommentActionsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetCommentActionsResponse message, length delimited. Does not implicitly {@link oj.biz.GetCommentActionsResponse.verify|verify} messages.
             * @param message GetCommentActionsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetCommentActionsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetCommentActionsResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetCommentActionsResponse & oj.biz.GetCommentActionsResponse.$Shape} GetCommentActionsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetCommentActionsResponse & oj.biz.GetCommentActionsResponse.$Shape;

            /**
             * Decodes a GetCommentActionsResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetCommentActionsResponse & oj.biz.GetCommentActionsResponse.$Shape} GetCommentActionsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetCommentActionsResponse & oj.biz.GetCommentActionsResponse.$Shape;

            /**
             * Verifies a GetCommentActionsResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetCommentActionsResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetCommentActionsResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetCommentActionsResponse;

            /**
             * Creates a plain object from a GetCommentActionsResponse message. Also converts values to other types if specified.
             * @param message GetCommentActionsResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetCommentActionsResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetCommentActionsResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetCommentActionsResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetCommentActionsResponse {

            /** Properties of a GetCommentActionsResponse. */
            interface $Properties {

                /** GetCommentActionsResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetCommentActionsResponse actions */
                actions?: (oj.common.CommentActionState.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetCommentActionsResponse. */
            type $Shape = oj.biz.GetCommentActionsResponse.$Properties;
        }

        /** Represents a CommentService */
        class CommentService extends $protobuf.rpc.Service {

            /**
             * Constructs a new CommentService service.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             */
            constructor(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean);

            /**
             * Creates new CommentService service using the specified rpc implementation.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             * @returns RPC service. Useful where requests and/or responses are streamed.
             */
            static create(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean): CommentService;

            /** Calls ListComments. */
            listComments: oj.biz.CommentService.ListComments;

            /** Calls ListReplies. */
            listReplies: oj.biz.CommentService.ListReplies;

            /** Calls CreateComment. */
            createComment: oj.biz.CommentService.CreateComment;

            /** Calls UpdateComment. */
            updateComment: oj.biz.CommentService.UpdateComment;

            /** Calls DeleteComment. */
            deleteComment: oj.biz.CommentService.DeleteComment;

            /** Calls ToggleLike. */
            toggleLike: oj.biz.CommentService.ToggleLike;

            /** Calls ToggleFavorite. */
            toggleFavorite: oj.biz.CommentService.ToggleFavorite;

            /** Calls GetActionStates. */
            getActionStates: oj.biz.CommentService.GetActionStates;
        }

        namespace CommentService {

            /**
             * Callback as used by {@link oj.biz.CommentService#listComments}.
             * @param error Error, if any
             * @param [response] ListCommentsResponse
             */
            type ListCommentsCallback = (error: (Error|null), response?: oj.biz.ListCommentsResponse) => void;

            /** Calls ListComments. */
            type ListComments = {
              (request: oj.biz.IListCommentsRequest, callback: oj.biz.CommentService.ListCommentsCallback): void;
              (request: oj.biz.IListCommentsRequest): Promise<oj.biz.ListCommentsResponse>;
              readonly name: "ListComments";
              readonly path: "/oj.biz.CommentService/ListComments";
              readonly requestType: "ListCommentsRequest";
              readonly responseType: "ListCommentsResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.CommentService#listReplies}.
             * @param error Error, if any
             * @param [response] ListCommentsResponse
             */
            type ListRepliesCallback = (error: (Error|null), response?: oj.biz.ListCommentsResponse) => void;

            /** Calls ListReplies. */
            type ListReplies = {
              (request: oj.biz.IListRepliesRequest, callback: oj.biz.CommentService.ListRepliesCallback): void;
              (request: oj.biz.IListRepliesRequest): Promise<oj.biz.ListCommentsResponse>;
              readonly name: "ListReplies";
              readonly path: "/oj.biz.CommentService/ListReplies";
              readonly requestType: "ListRepliesRequest";
              readonly responseType: "ListCommentsResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.CommentService#createComment}.
             * @param error Error, if any
             * @param [response] CommentResponse
             */
            type CreateCommentCallback = (error: (Error|null), response?: oj.biz.CommentResponse) => void;

            /** Calls CreateComment. */
            type CreateComment = {
              (request: oj.biz.ICreateCommentRequest, callback: oj.biz.CommentService.CreateCommentCallback): void;
              (request: oj.biz.ICreateCommentRequest): Promise<oj.biz.CommentResponse>;
              readonly name: "CreateComment";
              readonly path: "/oj.biz.CommentService/CreateComment";
              readonly requestType: "CreateCommentRequest";
              readonly responseType: "CommentResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.CommentService#updateComment}.
             * @param error Error, if any
             * @param [response] CommentResponse
             */
            type UpdateCommentCallback = (error: (Error|null), response?: oj.biz.CommentResponse) => void;

            /** Calls UpdateComment. */
            type UpdateComment = {
              (request: oj.biz.IUpdateCommentRequest, callback: oj.biz.CommentService.UpdateCommentCallback): void;
              (request: oj.biz.IUpdateCommentRequest): Promise<oj.biz.CommentResponse>;
              readonly name: "UpdateComment";
              readonly path: "/oj.biz.CommentService/UpdateComment";
              readonly requestType: "UpdateCommentRequest";
              readonly responseType: "CommentResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.CommentService#deleteComment}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type DeleteCommentCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls DeleteComment. */
            type DeleteComment = {
              (request: oj.biz.ICommentIdRequest, callback: oj.biz.CommentService.DeleteCommentCallback): void;
              (request: oj.biz.ICommentIdRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "DeleteComment";
              readonly path: "/oj.biz.CommentService/DeleteComment";
              readonly requestType: "CommentIdRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.CommentService#toggleLike}.
             * @param error Error, if any
             * @param [response] CommentActionResponse
             */
            type ToggleLikeCallback = (error: (Error|null), response?: oj.biz.CommentActionResponse) => void;

            /** Calls ToggleLike. */
            type ToggleLike = {
              (request: oj.biz.ICommentIdRequest, callback: oj.biz.CommentService.ToggleLikeCallback): void;
              (request: oj.biz.ICommentIdRequest): Promise<oj.biz.CommentActionResponse>;
              readonly name: "ToggleLike";
              readonly path: "/oj.biz.CommentService/ToggleLike";
              readonly requestType: "CommentIdRequest";
              readonly responseType: "CommentActionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.CommentService#toggleFavorite}.
             * @param error Error, if any
             * @param [response] CommentActionResponse
             */
            type ToggleFavoriteCallback = (error: (Error|null), response?: oj.biz.CommentActionResponse) => void;

            /** Calls ToggleFavorite. */
            type ToggleFavorite = {
              (request: oj.biz.ICommentIdRequest, callback: oj.biz.CommentService.ToggleFavoriteCallback): void;
              (request: oj.biz.ICommentIdRequest): Promise<oj.biz.CommentActionResponse>;
              readonly name: "ToggleFavorite";
              readonly path: "/oj.biz.CommentService/ToggleFavorite";
              readonly requestType: "CommentIdRequest";
              readonly responseType: "CommentActionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.CommentService#getActionStates}.
             * @param error Error, if any
             * @param [response] GetCommentActionsResponse
             */
            type GetActionStatesCallback = (error: (Error|null), response?: oj.biz.GetCommentActionsResponse) => void;

            /** Calls GetActionStates. */
            type GetActionStates = {
              (request: oj.biz.IGetCommentActionsRequest, callback: oj.biz.CommentService.GetActionStatesCallback): void;
              (request: oj.biz.IGetCommentActionsRequest): Promise<oj.biz.GetCommentActionsResponse>;
              readonly name: "GetActionStates";
              readonly path: "/oj.biz.CommentService/GetActionStates";
              readonly requestType: "GetCommentActionsRequest";
              readonly responseType: "GetCommentActionsResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };
        }

        /**
         * Properties of an AdminLoginRequest.
         * @deprecated Use oj.biz.AdminLoginRequest.$Properties instead.
         */
        interface IAdminLoginRequest extends oj.biz.AdminLoginRequest.$Properties {
        }

        /** Represents an AdminLoginRequest. */
        class AdminLoginRequest {

            /**
             * Constructs a new AdminLoginRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminLoginRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminLoginRequest adminId. */
            adminId: number;

            /** AdminLoginRequest password. */
            password: string;

            /**
             * Creates a new AdminLoginRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminLoginRequest instance
             */
            static create(properties: oj.biz.AdminLoginRequest.$Shape): oj.biz.AdminLoginRequest & oj.biz.AdminLoginRequest.$Shape;
            static create(properties?: oj.biz.AdminLoginRequest.$Properties): oj.biz.AdminLoginRequest;

            /**
             * Encodes the specified AdminLoginRequest message. Does not implicitly {@link oj.biz.AdminLoginRequest.verify|verify} messages.
             * @param message AdminLoginRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminLoginRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminLoginRequest message, length delimited. Does not implicitly {@link oj.biz.AdminLoginRequest.verify|verify} messages.
             * @param message AdminLoginRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminLoginRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminLoginRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminLoginRequest & oj.biz.AdminLoginRequest.$Shape} AdminLoginRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminLoginRequest & oj.biz.AdminLoginRequest.$Shape;

            /**
             * Decodes an AdminLoginRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminLoginRequest & oj.biz.AdminLoginRequest.$Shape} AdminLoginRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminLoginRequest & oj.biz.AdminLoginRequest.$Shape;

            /**
             * Verifies an AdminLoginRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminLoginRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminLoginRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminLoginRequest;

            /**
             * Creates a plain object from an AdminLoginRequest message. Also converts values to other types if specified.
             * @param message AdminLoginRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminLoginRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminLoginRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminLoginRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminLoginRequest {

            /** Properties of an AdminLoginRequest. */
            interface $Properties {

                /** AdminLoginRequest adminId */
                adminId?: (number|null);

                /** AdminLoginRequest password */
                password?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminLoginRequest. */
            type $Shape = oj.biz.AdminLoginRequest.$Properties;
        }

        /**
         * Properties of an AdminLoginResponse.
         * @deprecated Use oj.biz.AdminLoginResponse.$Properties instead.
         */
        interface IAdminLoginResponse extends oj.biz.AdminLoginResponse.$Properties {
        }

        /** Represents an AdminLoginResponse. */
        class AdminLoginResponse {

            /**
             * Constructs a new AdminLoginResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminLoginResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminLoginResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** AdminLoginResponse admin. */
            admin?: (oj.common.AdminAccount.$Properties|null);

            /**
             * Creates a new AdminLoginResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminLoginResponse instance
             */
            static create(properties: oj.biz.AdminLoginResponse.$Shape): oj.biz.AdminLoginResponse & oj.biz.AdminLoginResponse.$Shape;
            static create(properties?: oj.biz.AdminLoginResponse.$Properties): oj.biz.AdminLoginResponse;

            /**
             * Encodes the specified AdminLoginResponse message. Does not implicitly {@link oj.biz.AdminLoginResponse.verify|verify} messages.
             * @param message AdminLoginResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminLoginResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminLoginResponse message, length delimited. Does not implicitly {@link oj.biz.AdminLoginResponse.verify|verify} messages.
             * @param message AdminLoginResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminLoginResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminLoginResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminLoginResponse & oj.biz.AdminLoginResponse.$Shape} AdminLoginResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminLoginResponse & oj.biz.AdminLoginResponse.$Shape;

            /**
             * Decodes an AdminLoginResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminLoginResponse & oj.biz.AdminLoginResponse.$Shape} AdminLoginResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminLoginResponse & oj.biz.AdminLoginResponse.$Shape;

            /**
             * Verifies an AdminLoginResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminLoginResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminLoginResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminLoginResponse;

            /**
             * Creates a plain object from an AdminLoginResponse message. Also converts values to other types if specified.
             * @param message AdminLoginResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminLoginResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminLoginResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminLoginResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminLoginResponse {

            /** Properties of an AdminLoginResponse. */
            interface $Properties {

                /** AdminLoginResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** AdminLoginResponse admin */
                admin?: (oj.common.AdminAccount.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminLoginResponse. */
            type $Shape = oj.biz.AdminLoginResponse.$Properties;
        }

        /**
         * Properties of a GetVersionResponse.
         * @deprecated Use oj.biz.GetVersionResponse.$Properties instead.
         */
        interface IGetVersionResponse extends oj.biz.GetVersionResponse.$Properties {
        }

        /** Represents a GetVersionResponse. */
        class GetVersionResponse {

            /**
             * Constructs a new GetVersionResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetVersionResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetVersionResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetVersionResponse service. */
            service: string;

            /** GetVersionResponse version. */
            version: string;

            /** GetVersionResponse buildDate. */
            buildDate: string;

            /** GetVersionResponse buildTime. */
            buildTime: string;

            /**
             * Creates a new GetVersionResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetVersionResponse instance
             */
            static create(properties: oj.biz.GetVersionResponse.$Shape): oj.biz.GetVersionResponse & oj.biz.GetVersionResponse.$Shape;
            static create(properties?: oj.biz.GetVersionResponse.$Properties): oj.biz.GetVersionResponse;

            /**
             * Encodes the specified GetVersionResponse message. Does not implicitly {@link oj.biz.GetVersionResponse.verify|verify} messages.
             * @param message GetVersionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetVersionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetVersionResponse message, length delimited. Does not implicitly {@link oj.biz.GetVersionResponse.verify|verify} messages.
             * @param message GetVersionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetVersionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetVersionResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetVersionResponse & oj.biz.GetVersionResponse.$Shape} GetVersionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetVersionResponse & oj.biz.GetVersionResponse.$Shape;

            /**
             * Decodes a GetVersionResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetVersionResponse & oj.biz.GetVersionResponse.$Shape} GetVersionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetVersionResponse & oj.biz.GetVersionResponse.$Shape;

            /**
             * Verifies a GetVersionResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetVersionResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetVersionResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetVersionResponse;

            /**
             * Creates a plain object from a GetVersionResponse message. Also converts values to other types if specified.
             * @param message GetVersionResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetVersionResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetVersionResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetVersionResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetVersionResponse {

            /** Properties of a GetVersionResponse. */
            interface $Properties {

                /** GetVersionResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetVersionResponse service */
                service?: (string|null);

                /** GetVersionResponse version */
                version?: (string|null);

                /** GetVersionResponse buildDate */
                buildDate?: (string|null);

                /** GetVersionResponse buildTime */
                buildTime?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetVersionResponse. */
            type $Shape = oj.biz.GetVersionResponse.$Properties;
        }

        /**
         * Properties of an AdminOverviewResponse.
         * @deprecated Use oj.biz.AdminOverviewResponse.$Properties instead.
         */
        interface IAdminOverviewResponse extends oj.biz.AdminOverviewResponse.$Properties {
        }

        /** Represents an AdminOverviewResponse. */
        class AdminOverviewResponse {

            /**
             * Constructs a new AdminOverviewResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminOverviewResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminOverviewResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** AdminOverviewResponse totalUsers. */
            totalUsers: (Long|string);

            /** AdminOverviewResponse totalQuestions. */
            totalQuestions: (Long|string);

            /** AdminOverviewResponse totalSolutions. */
            totalSolutions: (Long|string);

            /** AdminOverviewResponse totalSubmissions. */
            totalSubmissions: (Long|string);

            /** AdminOverviewResponse todaySubmissions. */
            todaySubmissions: (Long|string);

            /** AdminOverviewResponse judgeServerCount. */
            judgeServerCount: number;

            /**
             * Creates a new AdminOverviewResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminOverviewResponse instance
             */
            static create(properties: oj.biz.AdminOverviewResponse.$Shape): oj.biz.AdminOverviewResponse & oj.biz.AdminOverviewResponse.$Shape;
            static create(properties?: oj.biz.AdminOverviewResponse.$Properties): oj.biz.AdminOverviewResponse;

            /**
             * Encodes the specified AdminOverviewResponse message. Does not implicitly {@link oj.biz.AdminOverviewResponse.verify|verify} messages.
             * @param message AdminOverviewResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminOverviewResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminOverviewResponse message, length delimited. Does not implicitly {@link oj.biz.AdminOverviewResponse.verify|verify} messages.
             * @param message AdminOverviewResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminOverviewResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminOverviewResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminOverviewResponse & oj.biz.AdminOverviewResponse.$Shape} AdminOverviewResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminOverviewResponse & oj.biz.AdminOverviewResponse.$Shape;

            /**
             * Decodes an AdminOverviewResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminOverviewResponse & oj.biz.AdminOverviewResponse.$Shape} AdminOverviewResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminOverviewResponse & oj.biz.AdminOverviewResponse.$Shape;

            /**
             * Verifies an AdminOverviewResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminOverviewResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminOverviewResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminOverviewResponse;

            /**
             * Creates a plain object from an AdminOverviewResponse message. Also converts values to other types if specified.
             * @param message AdminOverviewResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminOverviewResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminOverviewResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminOverviewResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminOverviewResponse {

            /** Properties of an AdminOverviewResponse. */
            interface $Properties {

                /** AdminOverviewResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** AdminOverviewResponse totalUsers */
                totalUsers?: ((Long|string)|null);

                /** AdminOverviewResponse totalQuestions */
                totalQuestions?: ((Long|string)|null);

                /** AdminOverviewResponse totalSolutions */
                totalSolutions?: ((Long|string)|null);

                /** AdminOverviewResponse totalSubmissions */
                totalSubmissions?: ((Long|string)|null);

                /** AdminOverviewResponse todaySubmissions */
                todaySubmissions?: ((Long|string)|null);

                /** AdminOverviewResponse judgeServerCount */
                judgeServerCount?: (number|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminOverviewResponse. */
            type $Shape = oj.biz.AdminOverviewResponse.$Properties;
        }

        /**
         * Properties of an AdminListUsersRequest.
         * @deprecated Use oj.biz.AdminListUsersRequest.$Properties instead.
         */
        interface IAdminListUsersRequest extends oj.biz.AdminListUsersRequest.$Properties {
        }

        /** Represents an AdminListUsersRequest. */
        class AdminListUsersRequest {

            /**
             * Constructs a new AdminListUsersRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminListUsersRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminListUsersRequest page. */
            page?: (oj.common.PageRequest.$Properties|null);

            /** AdminListUsersRequest search. */
            search: string;

            /**
             * Creates a new AdminListUsersRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminListUsersRequest instance
             */
            static create(properties: oj.biz.AdminListUsersRequest.$Shape): oj.biz.AdminListUsersRequest & oj.biz.AdminListUsersRequest.$Shape;
            static create(properties?: oj.biz.AdminListUsersRequest.$Properties): oj.biz.AdminListUsersRequest;

            /**
             * Encodes the specified AdminListUsersRequest message. Does not implicitly {@link oj.biz.AdminListUsersRequest.verify|verify} messages.
             * @param message AdminListUsersRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminListUsersRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminListUsersRequest message, length delimited. Does not implicitly {@link oj.biz.AdminListUsersRequest.verify|verify} messages.
             * @param message AdminListUsersRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminListUsersRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminListUsersRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminListUsersRequest & oj.biz.AdminListUsersRequest.$Shape} AdminListUsersRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminListUsersRequest & oj.biz.AdminListUsersRequest.$Shape;

            /**
             * Decodes an AdminListUsersRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminListUsersRequest & oj.biz.AdminListUsersRequest.$Shape} AdminListUsersRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminListUsersRequest & oj.biz.AdminListUsersRequest.$Shape;

            /**
             * Verifies an AdminListUsersRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminListUsersRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminListUsersRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminListUsersRequest;

            /**
             * Creates a plain object from an AdminListUsersRequest message. Also converts values to other types if specified.
             * @param message AdminListUsersRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminListUsersRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminListUsersRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminListUsersRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminListUsersRequest {

            /** Properties of an AdminListUsersRequest. */
            interface $Properties {

                /** AdminListUsersRequest page */
                page?: (oj.common.PageRequest.$Properties|null);

                /** AdminListUsersRequest search */
                search?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminListUsersRequest. */
            type $Shape = oj.biz.AdminListUsersRequest.$Properties;
        }

        /**
         * Properties of an AdminListUsersResponse.
         * @deprecated Use oj.biz.AdminListUsersResponse.$Properties instead.
         */
        interface IAdminListUsersResponse extends oj.biz.AdminListUsersResponse.$Properties {
        }

        /** Represents an AdminListUsersResponse. */
        class AdminListUsersResponse {

            /**
             * Constructs a new AdminListUsersResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminListUsersResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminListUsersResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** AdminListUsersResponse page. */
            page?: (oj.common.PageResponse.$Properties|null);

            /** AdminListUsersResponse users. */
            users: oj.common.User.$Properties[];

            /**
             * Creates a new AdminListUsersResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminListUsersResponse instance
             */
            static create(properties: oj.biz.AdminListUsersResponse.$Shape): oj.biz.AdminListUsersResponse & oj.biz.AdminListUsersResponse.$Shape;
            static create(properties?: oj.biz.AdminListUsersResponse.$Properties): oj.biz.AdminListUsersResponse;

            /**
             * Encodes the specified AdminListUsersResponse message. Does not implicitly {@link oj.biz.AdminListUsersResponse.verify|verify} messages.
             * @param message AdminListUsersResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminListUsersResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminListUsersResponse message, length delimited. Does not implicitly {@link oj.biz.AdminListUsersResponse.verify|verify} messages.
             * @param message AdminListUsersResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminListUsersResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminListUsersResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminListUsersResponse & oj.biz.AdminListUsersResponse.$Shape} AdminListUsersResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminListUsersResponse & oj.biz.AdminListUsersResponse.$Shape;

            /**
             * Decodes an AdminListUsersResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminListUsersResponse & oj.biz.AdminListUsersResponse.$Shape} AdminListUsersResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminListUsersResponse & oj.biz.AdminListUsersResponse.$Shape;

            /**
             * Verifies an AdminListUsersResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminListUsersResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminListUsersResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminListUsersResponse;

            /**
             * Creates a plain object from an AdminListUsersResponse message. Also converts values to other types if specified.
             * @param message AdminListUsersResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminListUsersResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminListUsersResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminListUsersResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminListUsersResponse {

            /** Properties of an AdminListUsersResponse. */
            interface $Properties {

                /** AdminListUsersResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** AdminListUsersResponse page */
                page?: (oj.common.PageResponse.$Properties|null);

                /** AdminListUsersResponse users */
                users?: (oj.common.User.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminListUsersResponse. */
            type $Shape = oj.biz.AdminListUsersResponse.$Properties;
        }

        /**
         * Properties of an AdminCreateUserRequest.
         * @deprecated Use oj.biz.AdminCreateUserRequest.$Properties instead.
         */
        interface IAdminCreateUserRequest extends oj.biz.AdminCreateUserRequest.$Properties {
        }

        /** Represents an AdminCreateUserRequest. */
        class AdminCreateUserRequest {

            /**
             * Constructs a new AdminCreateUserRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminCreateUserRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminCreateUserRequest user. */
            user?: (oj.common.User.$Properties|null);

            /** AdminCreateUserRequest password. */
            password: string;

            /**
             * Creates a new AdminCreateUserRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminCreateUserRequest instance
             */
            static create(properties: oj.biz.AdminCreateUserRequest.$Shape): oj.biz.AdminCreateUserRequest & oj.biz.AdminCreateUserRequest.$Shape;
            static create(properties?: oj.biz.AdminCreateUserRequest.$Properties): oj.biz.AdminCreateUserRequest;

            /**
             * Encodes the specified AdminCreateUserRequest message. Does not implicitly {@link oj.biz.AdminCreateUserRequest.verify|verify} messages.
             * @param message AdminCreateUserRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminCreateUserRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminCreateUserRequest message, length delimited. Does not implicitly {@link oj.biz.AdminCreateUserRequest.verify|verify} messages.
             * @param message AdminCreateUserRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminCreateUserRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminCreateUserRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminCreateUserRequest & oj.biz.AdminCreateUserRequest.$Shape} AdminCreateUserRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminCreateUserRequest & oj.biz.AdminCreateUserRequest.$Shape;

            /**
             * Decodes an AdminCreateUserRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminCreateUserRequest & oj.biz.AdminCreateUserRequest.$Shape} AdminCreateUserRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminCreateUserRequest & oj.biz.AdminCreateUserRequest.$Shape;

            /**
             * Verifies an AdminCreateUserRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminCreateUserRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminCreateUserRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminCreateUserRequest;

            /**
             * Creates a plain object from an AdminCreateUserRequest message. Also converts values to other types if specified.
             * @param message AdminCreateUserRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminCreateUserRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminCreateUserRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminCreateUserRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminCreateUserRequest {

            /** Properties of an AdminCreateUserRequest. */
            interface $Properties {

                /** AdminCreateUserRequest user */
                user?: (oj.common.User.$Properties|null);

                /** AdminCreateUserRequest password */
                password?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminCreateUserRequest. */
            type $Shape = oj.biz.AdminCreateUserRequest.$Properties;
        }

        /**
         * Properties of an AdminUpdateUserRequest.
         * @deprecated Use oj.biz.AdminUpdateUserRequest.$Properties instead.
         */
        interface IAdminUpdateUserRequest extends oj.biz.AdminUpdateUserRequest.$Properties {
        }

        /** Represents an AdminUpdateUserRequest. */
        class AdminUpdateUserRequest {

            /**
             * Constructs a new AdminUpdateUserRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminUpdateUserRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminUpdateUserRequest user. */
            user?: (oj.common.User.$Properties|null);

            /**
             * Creates a new AdminUpdateUserRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminUpdateUserRequest instance
             */
            static create(properties: oj.biz.AdminUpdateUserRequest.$Shape): oj.biz.AdminUpdateUserRequest & oj.biz.AdminUpdateUserRequest.$Shape;
            static create(properties?: oj.biz.AdminUpdateUserRequest.$Properties): oj.biz.AdminUpdateUserRequest;

            /**
             * Encodes the specified AdminUpdateUserRequest message. Does not implicitly {@link oj.biz.AdminUpdateUserRequest.verify|verify} messages.
             * @param message AdminUpdateUserRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminUpdateUserRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminUpdateUserRequest message, length delimited. Does not implicitly {@link oj.biz.AdminUpdateUserRequest.verify|verify} messages.
             * @param message AdminUpdateUserRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminUpdateUserRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminUpdateUserRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminUpdateUserRequest & oj.biz.AdminUpdateUserRequest.$Shape} AdminUpdateUserRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminUpdateUserRequest & oj.biz.AdminUpdateUserRequest.$Shape;

            /**
             * Decodes an AdminUpdateUserRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminUpdateUserRequest & oj.biz.AdminUpdateUserRequest.$Shape} AdminUpdateUserRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminUpdateUserRequest & oj.biz.AdminUpdateUserRequest.$Shape;

            /**
             * Verifies an AdminUpdateUserRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminUpdateUserRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminUpdateUserRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminUpdateUserRequest;

            /**
             * Creates a plain object from an AdminUpdateUserRequest message. Also converts values to other types if specified.
             * @param message AdminUpdateUserRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminUpdateUserRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminUpdateUserRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminUpdateUserRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminUpdateUserRequest {

            /** Properties of an AdminUpdateUserRequest. */
            interface $Properties {

                /** AdminUpdateUserRequest user */
                user?: (oj.common.User.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminUpdateUserRequest. */
            type $Shape = oj.biz.AdminUpdateUserRequest.$Properties;
        }

        /**
         * Properties of an AdminUserIdRequest.
         * @deprecated Use oj.biz.AdminUserIdRequest.$Properties instead.
         */
        interface IAdminUserIdRequest extends oj.biz.AdminUserIdRequest.$Properties {
        }

        /** Represents an AdminUserIdRequest. */
        class AdminUserIdRequest {

            /**
             * Constructs a new AdminUserIdRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminUserIdRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminUserIdRequest uid. */
            uid: number;

            /**
             * Creates a new AdminUserIdRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminUserIdRequest instance
             */
            static create(properties: oj.biz.AdminUserIdRequest.$Shape): oj.biz.AdminUserIdRequest & oj.biz.AdminUserIdRequest.$Shape;
            static create(properties?: oj.biz.AdminUserIdRequest.$Properties): oj.biz.AdminUserIdRequest;

            /**
             * Encodes the specified AdminUserIdRequest message. Does not implicitly {@link oj.biz.AdminUserIdRequest.verify|verify} messages.
             * @param message AdminUserIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminUserIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminUserIdRequest message, length delimited. Does not implicitly {@link oj.biz.AdminUserIdRequest.verify|verify} messages.
             * @param message AdminUserIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminUserIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminUserIdRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminUserIdRequest & oj.biz.AdminUserIdRequest.$Shape} AdminUserIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminUserIdRequest & oj.biz.AdminUserIdRequest.$Shape;

            /**
             * Decodes an AdminUserIdRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminUserIdRequest & oj.biz.AdminUserIdRequest.$Shape} AdminUserIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminUserIdRequest & oj.biz.AdminUserIdRequest.$Shape;

            /**
             * Verifies an AdminUserIdRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminUserIdRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminUserIdRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminUserIdRequest;

            /**
             * Creates a plain object from an AdminUserIdRequest message. Also converts values to other types if specified.
             * @param message AdminUserIdRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminUserIdRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminUserIdRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminUserIdRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminUserIdRequest {

            /** Properties of an AdminUserIdRequest. */
            interface $Properties {

                /** AdminUserIdRequest uid */
                uid?: (number|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminUserIdRequest. */
            type $Shape = oj.biz.AdminUserIdRequest.$Properties;
        }

        /**
         * Properties of an AdminUserResponse.
         * @deprecated Use oj.biz.AdminUserResponse.$Properties instead.
         */
        interface IAdminUserResponse extends oj.biz.AdminUserResponse.$Properties {
        }

        /** Represents an AdminUserResponse. */
        class AdminUserResponse {

            /**
             * Constructs a new AdminUserResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminUserResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminUserResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** AdminUserResponse user. */
            user?: (oj.common.User.$Properties|null);

            /**
             * Creates a new AdminUserResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminUserResponse instance
             */
            static create(properties: oj.biz.AdminUserResponse.$Shape): oj.biz.AdminUserResponse & oj.biz.AdminUserResponse.$Shape;
            static create(properties?: oj.biz.AdminUserResponse.$Properties): oj.biz.AdminUserResponse;

            /**
             * Encodes the specified AdminUserResponse message. Does not implicitly {@link oj.biz.AdminUserResponse.verify|verify} messages.
             * @param message AdminUserResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminUserResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminUserResponse message, length delimited. Does not implicitly {@link oj.biz.AdminUserResponse.verify|verify} messages.
             * @param message AdminUserResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminUserResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminUserResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminUserResponse & oj.biz.AdminUserResponse.$Shape} AdminUserResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminUserResponse & oj.biz.AdminUserResponse.$Shape;

            /**
             * Decodes an AdminUserResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminUserResponse & oj.biz.AdminUserResponse.$Shape} AdminUserResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminUserResponse & oj.biz.AdminUserResponse.$Shape;

            /**
             * Verifies an AdminUserResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminUserResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminUserResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminUserResponse;

            /**
             * Creates a plain object from an AdminUserResponse message. Also converts values to other types if specified.
             * @param message AdminUserResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminUserResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminUserResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminUserResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminUserResponse {

            /** Properties of an AdminUserResponse. */
            interface $Properties {

                /** AdminUserResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** AdminUserResponse user */
                user?: (oj.common.User.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminUserResponse. */
            type $Shape = oj.biz.AdminUserResponse.$Properties;
        }

        /**
         * Properties of a ListAdminAccountsRequest.
         * @deprecated Use oj.biz.ListAdminAccountsRequest.$Properties instead.
         */
        interface IListAdminAccountsRequest extends oj.biz.ListAdminAccountsRequest.$Properties {
        }

        /** Represents a ListAdminAccountsRequest. */
        class ListAdminAccountsRequest {

            /**
             * Constructs a new ListAdminAccountsRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListAdminAccountsRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListAdminAccountsRequest page. */
            page?: (oj.common.PageRequest.$Properties|null);

            /** ListAdminAccountsRequest search. */
            search: string;

            /**
             * Creates a new ListAdminAccountsRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListAdminAccountsRequest instance
             */
            static create(properties: oj.biz.ListAdminAccountsRequest.$Shape): oj.biz.ListAdminAccountsRequest & oj.biz.ListAdminAccountsRequest.$Shape;
            static create(properties?: oj.biz.ListAdminAccountsRequest.$Properties): oj.biz.ListAdminAccountsRequest;

            /**
             * Encodes the specified ListAdminAccountsRequest message. Does not implicitly {@link oj.biz.ListAdminAccountsRequest.verify|verify} messages.
             * @param message ListAdminAccountsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListAdminAccountsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListAdminAccountsRequest message, length delimited. Does not implicitly {@link oj.biz.ListAdminAccountsRequest.verify|verify} messages.
             * @param message ListAdminAccountsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListAdminAccountsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListAdminAccountsRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListAdminAccountsRequest & oj.biz.ListAdminAccountsRequest.$Shape} ListAdminAccountsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListAdminAccountsRequest & oj.biz.ListAdminAccountsRequest.$Shape;

            /**
             * Decodes a ListAdminAccountsRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListAdminAccountsRequest & oj.biz.ListAdminAccountsRequest.$Shape} ListAdminAccountsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListAdminAccountsRequest & oj.biz.ListAdminAccountsRequest.$Shape;

            /**
             * Verifies a ListAdminAccountsRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListAdminAccountsRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListAdminAccountsRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListAdminAccountsRequest;

            /**
             * Creates a plain object from a ListAdminAccountsRequest message. Also converts values to other types if specified.
             * @param message ListAdminAccountsRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListAdminAccountsRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListAdminAccountsRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListAdminAccountsRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListAdminAccountsRequest {

            /** Properties of a ListAdminAccountsRequest. */
            interface $Properties {

                /** ListAdminAccountsRequest page */
                page?: (oj.common.PageRequest.$Properties|null);

                /** ListAdminAccountsRequest search */
                search?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListAdminAccountsRequest. */
            type $Shape = oj.biz.ListAdminAccountsRequest.$Properties;
        }

        /**
         * Properties of a ListAdminAccountsResponse.
         * @deprecated Use oj.biz.ListAdminAccountsResponse.$Properties instead.
         */
        interface IListAdminAccountsResponse extends oj.biz.ListAdminAccountsResponse.$Properties {
        }

        /** Represents a ListAdminAccountsResponse. */
        class ListAdminAccountsResponse {

            /**
             * Constructs a new ListAdminAccountsResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListAdminAccountsResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListAdminAccountsResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** ListAdminAccountsResponse page. */
            page?: (oj.common.PageResponse.$Properties|null);

            /** ListAdminAccountsResponse accounts. */
            accounts: oj.common.AdminAccount.$Properties[];

            /**
             * Creates a new ListAdminAccountsResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListAdminAccountsResponse instance
             */
            static create(properties: oj.biz.ListAdminAccountsResponse.$Shape): oj.biz.ListAdminAccountsResponse & oj.biz.ListAdminAccountsResponse.$Shape;
            static create(properties?: oj.biz.ListAdminAccountsResponse.$Properties): oj.biz.ListAdminAccountsResponse;

            /**
             * Encodes the specified ListAdminAccountsResponse message. Does not implicitly {@link oj.biz.ListAdminAccountsResponse.verify|verify} messages.
             * @param message ListAdminAccountsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListAdminAccountsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListAdminAccountsResponse message, length delimited. Does not implicitly {@link oj.biz.ListAdminAccountsResponse.verify|verify} messages.
             * @param message ListAdminAccountsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListAdminAccountsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListAdminAccountsResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListAdminAccountsResponse & oj.biz.ListAdminAccountsResponse.$Shape} ListAdminAccountsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListAdminAccountsResponse & oj.biz.ListAdminAccountsResponse.$Shape;

            /**
             * Decodes a ListAdminAccountsResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListAdminAccountsResponse & oj.biz.ListAdminAccountsResponse.$Shape} ListAdminAccountsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListAdminAccountsResponse & oj.biz.ListAdminAccountsResponse.$Shape;

            /**
             * Verifies a ListAdminAccountsResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListAdminAccountsResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListAdminAccountsResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListAdminAccountsResponse;

            /**
             * Creates a plain object from a ListAdminAccountsResponse message. Also converts values to other types if specified.
             * @param message ListAdminAccountsResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListAdminAccountsResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListAdminAccountsResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListAdminAccountsResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListAdminAccountsResponse {

            /** Properties of a ListAdminAccountsResponse. */
            interface $Properties {

                /** ListAdminAccountsResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** ListAdminAccountsResponse page */
                page?: (oj.common.PageResponse.$Properties|null);

                /** ListAdminAccountsResponse accounts */
                accounts?: (oj.common.AdminAccount.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListAdminAccountsResponse. */
            type $Shape = oj.biz.ListAdminAccountsResponse.$Properties;
        }

        /**
         * Properties of a SaveAdminAccountRequest.
         * @deprecated Use oj.biz.SaveAdminAccountRequest.$Properties instead.
         */
        interface ISaveAdminAccountRequest extends oj.biz.SaveAdminAccountRequest.$Properties {
        }

        /** Represents a SaveAdminAccountRequest. */
        class SaveAdminAccountRequest {

            /**
             * Constructs a new SaveAdminAccountRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.SaveAdminAccountRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** SaveAdminAccountRequest account. */
            account?: (oj.common.AdminAccount.$Properties|null);

            /** SaveAdminAccountRequest password. */
            password: string;

            /**
             * Creates a new SaveAdminAccountRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns SaveAdminAccountRequest instance
             */
            static create(properties: oj.biz.SaveAdminAccountRequest.$Shape): oj.biz.SaveAdminAccountRequest & oj.biz.SaveAdminAccountRequest.$Shape;
            static create(properties?: oj.biz.SaveAdminAccountRequest.$Properties): oj.biz.SaveAdminAccountRequest;

            /**
             * Encodes the specified SaveAdminAccountRequest message. Does not implicitly {@link oj.biz.SaveAdminAccountRequest.verify|verify} messages.
             * @param message SaveAdminAccountRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.SaveAdminAccountRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified SaveAdminAccountRequest message, length delimited. Does not implicitly {@link oj.biz.SaveAdminAccountRequest.verify|verify} messages.
             * @param message SaveAdminAccountRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.SaveAdminAccountRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a SaveAdminAccountRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.SaveAdminAccountRequest & oj.biz.SaveAdminAccountRequest.$Shape} SaveAdminAccountRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.SaveAdminAccountRequest & oj.biz.SaveAdminAccountRequest.$Shape;

            /**
             * Decodes a SaveAdminAccountRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.SaveAdminAccountRequest & oj.biz.SaveAdminAccountRequest.$Shape} SaveAdminAccountRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.SaveAdminAccountRequest & oj.biz.SaveAdminAccountRequest.$Shape;

            /**
             * Verifies a SaveAdminAccountRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a SaveAdminAccountRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns SaveAdminAccountRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.SaveAdminAccountRequest;

            /**
             * Creates a plain object from a SaveAdminAccountRequest message. Also converts values to other types if specified.
             * @param message SaveAdminAccountRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.SaveAdminAccountRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this SaveAdminAccountRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for SaveAdminAccountRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace SaveAdminAccountRequest {

            /** Properties of a SaveAdminAccountRequest. */
            interface $Properties {

                /** SaveAdminAccountRequest account */
                account?: (oj.common.AdminAccount.$Properties|null);

                /** SaveAdminAccountRequest password */
                password?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a SaveAdminAccountRequest. */
            type $Shape = oj.biz.SaveAdminAccountRequest.$Properties;
        }

        /**
         * Properties of an AdminAccountIdRequest.
         * @deprecated Use oj.biz.AdminAccountIdRequest.$Properties instead.
         */
        interface IAdminAccountIdRequest extends oj.biz.AdminAccountIdRequest.$Properties {
        }

        /** Represents an AdminAccountIdRequest. */
        class AdminAccountIdRequest {

            /**
             * Constructs a new AdminAccountIdRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminAccountIdRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminAccountIdRequest adminId. */
            adminId: number;

            /**
             * Creates a new AdminAccountIdRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminAccountIdRequest instance
             */
            static create(properties: oj.biz.AdminAccountIdRequest.$Shape): oj.biz.AdminAccountIdRequest & oj.biz.AdminAccountIdRequest.$Shape;
            static create(properties?: oj.biz.AdminAccountIdRequest.$Properties): oj.biz.AdminAccountIdRequest;

            /**
             * Encodes the specified AdminAccountIdRequest message. Does not implicitly {@link oj.biz.AdminAccountIdRequest.verify|verify} messages.
             * @param message AdminAccountIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminAccountIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminAccountIdRequest message, length delimited. Does not implicitly {@link oj.biz.AdminAccountIdRequest.verify|verify} messages.
             * @param message AdminAccountIdRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminAccountIdRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminAccountIdRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminAccountIdRequest & oj.biz.AdminAccountIdRequest.$Shape} AdminAccountIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminAccountIdRequest & oj.biz.AdminAccountIdRequest.$Shape;

            /**
             * Decodes an AdminAccountIdRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminAccountIdRequest & oj.biz.AdminAccountIdRequest.$Shape} AdminAccountIdRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminAccountIdRequest & oj.biz.AdminAccountIdRequest.$Shape;

            /**
             * Verifies an AdminAccountIdRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminAccountIdRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminAccountIdRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminAccountIdRequest;

            /**
             * Creates a plain object from an AdminAccountIdRequest message. Also converts values to other types if specified.
             * @param message AdminAccountIdRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminAccountIdRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminAccountIdRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminAccountIdRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminAccountIdRequest {

            /** Properties of an AdminAccountIdRequest. */
            interface $Properties {

                /** AdminAccountIdRequest adminId */
                adminId?: (number|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminAccountIdRequest. */
            type $Shape = oj.biz.AdminAccountIdRequest.$Properties;
        }

        /**
         * Properties of an AdminAccountResponse.
         * @deprecated Use oj.biz.AdminAccountResponse.$Properties instead.
         */
        interface IAdminAccountResponse extends oj.biz.AdminAccountResponse.$Properties {
        }

        /** Represents an AdminAccountResponse. */
        class AdminAccountResponse {

            /**
             * Constructs a new AdminAccountResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.AdminAccountResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminAccountResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** AdminAccountResponse account. */
            account?: (oj.common.AdminAccount.$Properties|null);

            /**
             * Creates a new AdminAccountResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminAccountResponse instance
             */
            static create(properties: oj.biz.AdminAccountResponse.$Shape): oj.biz.AdminAccountResponse & oj.biz.AdminAccountResponse.$Shape;
            static create(properties?: oj.biz.AdminAccountResponse.$Properties): oj.biz.AdminAccountResponse;

            /**
             * Encodes the specified AdminAccountResponse message. Does not implicitly {@link oj.biz.AdminAccountResponse.verify|verify} messages.
             * @param message AdminAccountResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.AdminAccountResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminAccountResponse message, length delimited. Does not implicitly {@link oj.biz.AdminAccountResponse.verify|verify} messages.
             * @param message AdminAccountResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.AdminAccountResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminAccountResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.AdminAccountResponse & oj.biz.AdminAccountResponse.$Shape} AdminAccountResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.AdminAccountResponse & oj.biz.AdminAccountResponse.$Shape;

            /**
             * Decodes an AdminAccountResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.AdminAccountResponse & oj.biz.AdminAccountResponse.$Shape} AdminAccountResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.AdminAccountResponse & oj.biz.AdminAccountResponse.$Shape;

            /**
             * Verifies an AdminAccountResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminAccountResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminAccountResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.AdminAccountResponse;

            /**
             * Creates a plain object from an AdminAccountResponse message. Also converts values to other types if specified.
             * @param message AdminAccountResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.AdminAccountResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminAccountResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminAccountResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminAccountResponse {

            /** Properties of an AdminAccountResponse. */
            interface $Properties {

                /** AdminAccountResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** AdminAccountResponse account */
                account?: (oj.common.AdminAccount.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminAccountResponse. */
            type $Shape = oj.biz.AdminAccountResponse.$Properties;
        }

        /**
         * Properties of a GetCacheMetricsResponse.
         * @deprecated Use oj.biz.GetCacheMetricsResponse.$Properties instead.
         */
        interface IGetCacheMetricsResponse extends oj.biz.GetCacheMetricsResponse.$Properties {
        }

        /** Represents a GetCacheMetricsResponse. */
        class GetCacheMetricsResponse {

            /**
             * Constructs a new GetCacheMetricsResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetCacheMetricsResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetCacheMetricsResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetCacheMetricsResponse metrics. */
            metrics?: (oj.common.CacheMetrics.$Properties|null);

            /**
             * Creates a new GetCacheMetricsResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetCacheMetricsResponse instance
             */
            static create(properties: oj.biz.GetCacheMetricsResponse.$Shape): oj.biz.GetCacheMetricsResponse & oj.biz.GetCacheMetricsResponse.$Shape;
            static create(properties?: oj.biz.GetCacheMetricsResponse.$Properties): oj.biz.GetCacheMetricsResponse;

            /**
             * Encodes the specified GetCacheMetricsResponse message. Does not implicitly {@link oj.biz.GetCacheMetricsResponse.verify|verify} messages.
             * @param message GetCacheMetricsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetCacheMetricsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetCacheMetricsResponse message, length delimited. Does not implicitly {@link oj.biz.GetCacheMetricsResponse.verify|verify} messages.
             * @param message GetCacheMetricsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetCacheMetricsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetCacheMetricsResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetCacheMetricsResponse & oj.biz.GetCacheMetricsResponse.$Shape} GetCacheMetricsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetCacheMetricsResponse & oj.biz.GetCacheMetricsResponse.$Shape;

            /**
             * Decodes a GetCacheMetricsResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetCacheMetricsResponse & oj.biz.GetCacheMetricsResponse.$Shape} GetCacheMetricsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetCacheMetricsResponse & oj.biz.GetCacheMetricsResponse.$Shape;

            /**
             * Verifies a GetCacheMetricsResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetCacheMetricsResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetCacheMetricsResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetCacheMetricsResponse;

            /**
             * Creates a plain object from a GetCacheMetricsResponse message. Also converts values to other types if specified.
             * @param message GetCacheMetricsResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetCacheMetricsResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetCacheMetricsResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetCacheMetricsResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetCacheMetricsResponse {

            /** Properties of a GetCacheMetricsResponse. */
            interface $Properties {

                /** GetCacheMetricsResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetCacheMetricsResponse metrics */
                metrics?: (oj.common.CacheMetrics.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetCacheMetricsResponse. */
            type $Shape = oj.biz.GetCacheMetricsResponse.$Properties;
        }

        /**
         * Properties of a GetAuditLogsRequest.
         * @deprecated Use oj.biz.GetAuditLogsRequest.$Properties instead.
         */
        interface IGetAuditLogsRequest extends oj.biz.GetAuditLogsRequest.$Properties {
        }

        /** Represents a GetAuditLogsRequest. */
        class GetAuditLogsRequest {

            /**
             * Constructs a new GetAuditLogsRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetAuditLogsRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetAuditLogsRequest page. */
            page?: (oj.common.PageRequest.$Properties|null);

            /** GetAuditLogsRequest action. */
            action: string;

            /** GetAuditLogsRequest startTime. */
            startTime: (Long|string);

            /** GetAuditLogsRequest endTime. */
            endTime: (Long|string);

            /**
             * Creates a new GetAuditLogsRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetAuditLogsRequest instance
             */
            static create(properties: oj.biz.GetAuditLogsRequest.$Shape): oj.biz.GetAuditLogsRequest & oj.biz.GetAuditLogsRequest.$Shape;
            static create(properties?: oj.biz.GetAuditLogsRequest.$Properties): oj.biz.GetAuditLogsRequest;

            /**
             * Encodes the specified GetAuditLogsRequest message. Does not implicitly {@link oj.biz.GetAuditLogsRequest.verify|verify} messages.
             * @param message GetAuditLogsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetAuditLogsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetAuditLogsRequest message, length delimited. Does not implicitly {@link oj.biz.GetAuditLogsRequest.verify|verify} messages.
             * @param message GetAuditLogsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetAuditLogsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetAuditLogsRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetAuditLogsRequest & oj.biz.GetAuditLogsRequest.$Shape} GetAuditLogsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetAuditLogsRequest & oj.biz.GetAuditLogsRequest.$Shape;

            /**
             * Decodes a GetAuditLogsRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetAuditLogsRequest & oj.biz.GetAuditLogsRequest.$Shape} GetAuditLogsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetAuditLogsRequest & oj.biz.GetAuditLogsRequest.$Shape;

            /**
             * Verifies a GetAuditLogsRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetAuditLogsRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetAuditLogsRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetAuditLogsRequest;

            /**
             * Creates a plain object from a GetAuditLogsRequest message. Also converts values to other types if specified.
             * @param message GetAuditLogsRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetAuditLogsRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetAuditLogsRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetAuditLogsRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetAuditLogsRequest {

            /** Properties of a GetAuditLogsRequest. */
            interface $Properties {

                /** GetAuditLogsRequest page */
                page?: (oj.common.PageRequest.$Properties|null);

                /** GetAuditLogsRequest action */
                action?: (string|null);

                /** GetAuditLogsRequest startTime */
                startTime?: ((Long|string)|null);

                /** GetAuditLogsRequest endTime */
                endTime?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetAuditLogsRequest. */
            type $Shape = oj.biz.GetAuditLogsRequest.$Properties;
        }

        /**
         * Properties of a GetAuditLogsResponse.
         * @deprecated Use oj.biz.GetAuditLogsResponse.$Properties instead.
         */
        interface IGetAuditLogsResponse extends oj.biz.GetAuditLogsResponse.$Properties {
        }

        /** Represents a GetAuditLogsResponse. */
        class GetAuditLogsResponse {

            /**
             * Constructs a new GetAuditLogsResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetAuditLogsResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetAuditLogsResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetAuditLogsResponse page. */
            page?: (oj.common.PageResponse.$Properties|null);

            /** GetAuditLogsResponse logs. */
            logs: oj.common.AdminAuditLog.$Properties[];

            /**
             * Creates a new GetAuditLogsResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetAuditLogsResponse instance
             */
            static create(properties: oj.biz.GetAuditLogsResponse.$Shape): oj.biz.GetAuditLogsResponse & oj.biz.GetAuditLogsResponse.$Shape;
            static create(properties?: oj.biz.GetAuditLogsResponse.$Properties): oj.biz.GetAuditLogsResponse;

            /**
             * Encodes the specified GetAuditLogsResponse message. Does not implicitly {@link oj.biz.GetAuditLogsResponse.verify|verify} messages.
             * @param message GetAuditLogsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetAuditLogsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetAuditLogsResponse message, length delimited. Does not implicitly {@link oj.biz.GetAuditLogsResponse.verify|verify} messages.
             * @param message GetAuditLogsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetAuditLogsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetAuditLogsResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetAuditLogsResponse & oj.biz.GetAuditLogsResponse.$Shape} GetAuditLogsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetAuditLogsResponse & oj.biz.GetAuditLogsResponse.$Shape;

            /**
             * Decodes a GetAuditLogsResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetAuditLogsResponse & oj.biz.GetAuditLogsResponse.$Shape} GetAuditLogsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetAuditLogsResponse & oj.biz.GetAuditLogsResponse.$Shape;

            /**
             * Verifies a GetAuditLogsResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetAuditLogsResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetAuditLogsResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetAuditLogsResponse;

            /**
             * Creates a plain object from a GetAuditLogsResponse message. Also converts values to other types if specified.
             * @param message GetAuditLogsResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetAuditLogsResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetAuditLogsResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetAuditLogsResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetAuditLogsResponse {

            /** Properties of a GetAuditLogsResponse. */
            interface $Properties {

                /** GetAuditLogsResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetAuditLogsResponse page */
                page?: (oj.common.PageResponse.$Properties|null);

                /** GetAuditLogsResponse logs */
                logs?: (oj.common.AdminAuditLog.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetAuditLogsResponse. */
            type $Shape = oj.biz.GetAuditLogsResponse.$Properties;
        }

        /** Represents an AdminService */
        class AdminService extends $protobuf.rpc.Service {

            /**
             * Constructs a new AdminService service.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             */
            constructor(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean);

            /**
             * Creates new AdminService service using the specified rpc implementation.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             * @returns RPC service. Useful where requests and/or responses are streamed.
             */
            static create(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean): AdminService;

            /** Calls GetVersion. */
            getVersion: oj.biz.AdminService.GetVersion;

            /** Calls Login. */
            login: oj.biz.AdminService.Login;

            /** Calls Logout. */
            logout: oj.biz.AdminService.Logout;

            /** Calls GetOverview. */
            getOverview: oj.biz.AdminService.GetOverview;

            /** Calls ListUsers. */
            listUsers: oj.biz.AdminService.ListUsers;

            /** Calls GetUser. */
            getUser: oj.biz.AdminService.GetUser;

            /** Calls CreateUser. */
            createUser: oj.biz.AdminService.CreateUser;

            /** Calls UpdateUser. */
            updateUser: oj.biz.AdminService.UpdateUser;

            /** Calls DeleteUser. */
            deleteUser: oj.biz.AdminService.DeleteUser;

            /** Calls ListQuestions. */
            listQuestions: oj.biz.AdminService.ListQuestions;

            /** Calls GetQuestion. */
            getQuestion: oj.biz.AdminService.GetQuestion;

            /** Calls CreateQuestion. */
            createQuestion: oj.biz.AdminService.CreateQuestion;

            /** Calls UpdateQuestion. */
            updateQuestion: oj.biz.AdminService.UpdateQuestion;

            /** Calls DeleteQuestion. */
            deleteQuestion: oj.biz.AdminService.DeleteQuestion;

            /** Calls ListTestCases. */
            listTestCases: oj.biz.AdminService.ListTestCases;

            /** Calls CreateTestCase. */
            createTestCase: oj.biz.AdminService.CreateTestCase;

            /** Calls UpdateTestCase. */
            updateTestCase: oj.biz.AdminService.UpdateTestCase;

            /** Calls DeleteTestCase. */
            deleteTestCase: oj.biz.AdminService.DeleteTestCase;

            /** Calls InvalidateQuestionCache. */
            invalidateQuestionCache: oj.biz.AdminService.InvalidateQuestionCache;

            /** Calls ListAdminAccounts. */
            listAdminAccounts: oj.biz.AdminService.ListAdminAccounts;

            /** Calls GetAdminAccount. */
            getAdminAccount: oj.biz.AdminService.GetAdminAccount;

            /** Calls CreateAdminAccount. */
            createAdminAccount: oj.biz.AdminService.CreateAdminAccount;

            /** Calls UpdateAdminAccount. */
            updateAdminAccount: oj.biz.AdminService.UpdateAdminAccount;

            /** Calls DeleteAdminAccount. */
            deleteAdminAccount: oj.biz.AdminService.DeleteAdminAccount;

            /** Calls GetCacheMetrics. */
            getCacheMetrics: oj.biz.AdminService.GetCacheMetrics;

            /** Calls GetAuditLogs. */
            getAuditLogs: oj.biz.AdminService.GetAuditLogs;
        }

        namespace AdminService {

            /**
             * Callback as used by {@link oj.biz.AdminService#getVersion}.
             * @param error Error, if any
             * @param [response] GetVersionResponse
             */
            type GetVersionCallback = (error: (Error|null), response?: oj.biz.GetVersionResponse) => void;

            /** Calls GetVersion. */
            type GetVersion = {
              (request: oj.common.IEmptyRequest, callback: oj.biz.AdminService.GetVersionCallback): void;
              (request: oj.common.IEmptyRequest): Promise<oj.biz.GetVersionResponse>;
              readonly name: "GetVersion";
              readonly path: "/oj.biz.AdminService/GetVersion";
              readonly requestType: "oj.common.EmptyRequest";
              readonly responseType: "GetVersionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#login}.
             * @param error Error, if any
             * @param [response] AdminLoginResponse
             */
            type LoginCallback = (error: (Error|null), response?: oj.biz.AdminLoginResponse) => void;

            /** Calls Login. */
            type Login = {
              (request: oj.biz.IAdminLoginRequest, callback: oj.biz.AdminService.LoginCallback): void;
              (request: oj.biz.IAdminLoginRequest): Promise<oj.biz.AdminLoginResponse>;
              readonly name: "Login";
              readonly path: "/oj.biz.AdminService/Login";
              readonly requestType: "AdminLoginRequest";
              readonly responseType: "AdminLoginResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#logout}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type LogoutCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls Logout. */
            type Logout = {
              (request: oj.common.IEmptyRequest, callback: oj.biz.AdminService.LogoutCallback): void;
              (request: oj.common.IEmptyRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "Logout";
              readonly path: "/oj.biz.AdminService/Logout";
              readonly requestType: "oj.common.EmptyRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#getOverview}.
             * @param error Error, if any
             * @param [response] AdminOverviewResponse
             */
            type GetOverviewCallback = (error: (Error|null), response?: oj.biz.AdminOverviewResponse) => void;

            /** Calls GetOverview. */
            type GetOverview = {
              (request: oj.common.IEmptyRequest, callback: oj.biz.AdminService.GetOverviewCallback): void;
              (request: oj.common.IEmptyRequest): Promise<oj.biz.AdminOverviewResponse>;
              readonly name: "GetOverview";
              readonly path: "/oj.biz.AdminService/GetOverview";
              readonly requestType: "oj.common.EmptyRequest";
              readonly responseType: "AdminOverviewResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#listUsers}.
             * @param error Error, if any
             * @param [response] AdminListUsersResponse
             */
            type ListUsersCallback = (error: (Error|null), response?: oj.biz.AdminListUsersResponse) => void;

            /** Calls ListUsers. */
            type ListUsers = {
              (request: oj.biz.IAdminListUsersRequest, callback: oj.biz.AdminService.ListUsersCallback): void;
              (request: oj.biz.IAdminListUsersRequest): Promise<oj.biz.AdminListUsersResponse>;
              readonly name: "ListUsers";
              readonly path: "/oj.biz.AdminService/ListUsers";
              readonly requestType: "AdminListUsersRequest";
              readonly responseType: "AdminListUsersResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#getUser}.
             * @param error Error, if any
             * @param [response] AdminUserResponse
             */
            type GetUserCallback = (error: (Error|null), response?: oj.biz.AdminUserResponse) => void;

            /** Calls GetUser. */
            type GetUser = {
              (request: oj.biz.IAdminUserIdRequest, callback: oj.biz.AdminService.GetUserCallback): void;
              (request: oj.biz.IAdminUserIdRequest): Promise<oj.biz.AdminUserResponse>;
              readonly name: "GetUser";
              readonly path: "/oj.biz.AdminService/GetUser";
              readonly requestType: "AdminUserIdRequest";
              readonly responseType: "AdminUserResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#createUser}.
             * @param error Error, if any
             * @param [response] AdminUserResponse
             */
            type CreateUserCallback = (error: (Error|null), response?: oj.biz.AdminUserResponse) => void;

            /** Calls CreateUser. */
            type CreateUser = {
              (request: oj.biz.IAdminCreateUserRequest, callback: oj.biz.AdminService.CreateUserCallback): void;
              (request: oj.biz.IAdminCreateUserRequest): Promise<oj.biz.AdminUserResponse>;
              readonly name: "CreateUser";
              readonly path: "/oj.biz.AdminService/CreateUser";
              readonly requestType: "AdminCreateUserRequest";
              readonly responseType: "AdminUserResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#updateUser}.
             * @param error Error, if any
             * @param [response] AdminUserResponse
             */
            type UpdateUserCallback = (error: (Error|null), response?: oj.biz.AdminUserResponse) => void;

            /** Calls UpdateUser. */
            type UpdateUser = {
              (request: oj.biz.IAdminUpdateUserRequest, callback: oj.biz.AdminService.UpdateUserCallback): void;
              (request: oj.biz.IAdminUpdateUserRequest): Promise<oj.biz.AdminUserResponse>;
              readonly name: "UpdateUser";
              readonly path: "/oj.biz.AdminService/UpdateUser";
              readonly requestType: "AdminUpdateUserRequest";
              readonly responseType: "AdminUserResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#deleteUser}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type DeleteUserCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls DeleteUser. */
            type DeleteUser = {
              (request: oj.biz.IAdminUserIdRequest, callback: oj.biz.AdminService.DeleteUserCallback): void;
              (request: oj.biz.IAdminUserIdRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "DeleteUser";
              readonly path: "/oj.biz.AdminService/DeleteUser";
              readonly requestType: "AdminUserIdRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#listQuestions}.
             * @param error Error, if any
             * @param [response] GetQuestionListResponse
             */
            type ListQuestionsCallback = (error: (Error|null), response?: oj.biz.GetQuestionListResponse) => void;

            /** Calls ListQuestions. */
            type ListQuestions = {
              (request: oj.biz.IGetQuestionListRequest, callback: oj.biz.AdminService.ListQuestionsCallback): void;
              (request: oj.biz.IGetQuestionListRequest): Promise<oj.biz.GetQuestionListResponse>;
              readonly name: "ListQuestions";
              readonly path: "/oj.biz.AdminService/ListQuestions";
              readonly requestType: "GetQuestionListRequest";
              readonly responseType: "GetQuestionListResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#getQuestion}.
             * @param error Error, if any
             * @param [response] GetQuestionDetailResponse
             */
            type GetQuestionCallback = (error: (Error|null), response?: oj.biz.GetQuestionDetailResponse) => void;

            /** Calls GetQuestion. */
            type GetQuestion = {
              (request: oj.biz.IQuestionIdRequest, callback: oj.biz.AdminService.GetQuestionCallback): void;
              (request: oj.biz.IQuestionIdRequest): Promise<oj.biz.GetQuestionDetailResponse>;
              readonly name: "GetQuestion";
              readonly path: "/oj.biz.AdminService/GetQuestion";
              readonly requestType: "QuestionIdRequest";
              readonly responseType: "GetQuestionDetailResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#createQuestion}.
             * @param error Error, if any
             * @param [response] QuestionResponse
             */
            type CreateQuestionCallback = (error: (Error|null), response?: oj.biz.QuestionResponse) => void;

            /** Calls CreateQuestion. */
            type CreateQuestion = {
              (request: oj.biz.ISaveQuestionRequest, callback: oj.biz.AdminService.CreateQuestionCallback): void;
              (request: oj.biz.ISaveQuestionRequest): Promise<oj.biz.QuestionResponse>;
              readonly name: "CreateQuestion";
              readonly path: "/oj.biz.AdminService/CreateQuestion";
              readonly requestType: "SaveQuestionRequest";
              readonly responseType: "QuestionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#updateQuestion}.
             * @param error Error, if any
             * @param [response] QuestionResponse
             */
            type UpdateQuestionCallback = (error: (Error|null), response?: oj.biz.QuestionResponse) => void;

            /** Calls UpdateQuestion. */
            type UpdateQuestion = {
              (request: oj.biz.ISaveQuestionRequest, callback: oj.biz.AdminService.UpdateQuestionCallback): void;
              (request: oj.biz.ISaveQuestionRequest): Promise<oj.biz.QuestionResponse>;
              readonly name: "UpdateQuestion";
              readonly path: "/oj.biz.AdminService/UpdateQuestion";
              readonly requestType: "SaveQuestionRequest";
              readonly responseType: "QuestionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#deleteQuestion}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type DeleteQuestionCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls DeleteQuestion. */
            type DeleteQuestion = {
              (request: oj.biz.IQuestionIdRequest, callback: oj.biz.AdminService.DeleteQuestionCallback): void;
              (request: oj.biz.IQuestionIdRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "DeleteQuestion";
              readonly path: "/oj.biz.AdminService/DeleteQuestion";
              readonly requestType: "QuestionIdRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#listTestCases}.
             * @param error Error, if any
             * @param [response] ListTestCasesResponse
             */
            type ListTestCasesCallback = (error: (Error|null), response?: oj.biz.ListTestCasesResponse) => void;

            /** Calls ListTestCases. */
            type ListTestCases = {
              (request: oj.biz.IListTestCasesRequest, callback: oj.biz.AdminService.ListTestCasesCallback): void;
              (request: oj.biz.IListTestCasesRequest): Promise<oj.biz.ListTestCasesResponse>;
              readonly name: "ListTestCases";
              readonly path: "/oj.biz.AdminService/ListTestCases";
              readonly requestType: "ListTestCasesRequest";
              readonly responseType: "ListTestCasesResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#createTestCase}.
             * @param error Error, if any
             * @param [response] TestCaseResponse
             */
            type CreateTestCaseCallback = (error: (Error|null), response?: oj.biz.TestCaseResponse) => void;

            /** Calls CreateTestCase. */
            type CreateTestCase = {
              (request: oj.biz.ICreateTestCaseRequest, callback: oj.biz.AdminService.CreateTestCaseCallback): void;
              (request: oj.biz.ICreateTestCaseRequest): Promise<oj.biz.TestCaseResponse>;
              readonly name: "CreateTestCase";
              readonly path: "/oj.biz.AdminService/CreateTestCase";
              readonly requestType: "CreateTestCaseRequest";
              readonly responseType: "TestCaseResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#updateTestCase}.
             * @param error Error, if any
             * @param [response] TestCaseResponse
             */
            type UpdateTestCaseCallback = (error: (Error|null), response?: oj.biz.TestCaseResponse) => void;

            /** Calls UpdateTestCase. */
            type UpdateTestCase = {
              (request: oj.biz.IUpdateTestCaseRequest, callback: oj.biz.AdminService.UpdateTestCaseCallback): void;
              (request: oj.biz.IUpdateTestCaseRequest): Promise<oj.biz.TestCaseResponse>;
              readonly name: "UpdateTestCase";
              readonly path: "/oj.biz.AdminService/UpdateTestCase";
              readonly requestType: "UpdateTestCaseRequest";
              readonly responseType: "TestCaseResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#deleteTestCase}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type DeleteTestCaseCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls DeleteTestCase. */
            type DeleteTestCase = {
              (request: oj.biz.ITestCaseIdRequest, callback: oj.biz.AdminService.DeleteTestCaseCallback): void;
              (request: oj.biz.ITestCaseIdRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "DeleteTestCase";
              readonly path: "/oj.biz.AdminService/DeleteTestCase";
              readonly requestType: "TestCaseIdRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#invalidateQuestionCache}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type InvalidateQuestionCacheCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls InvalidateQuestionCache. */
            type InvalidateQuestionCache = {
              (request: oj.biz.IInvalidateQuestionCacheRequest, callback: oj.biz.AdminService.InvalidateQuestionCacheCallback): void;
              (request: oj.biz.IInvalidateQuestionCacheRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "InvalidateQuestionCache";
              readonly path: "/oj.biz.AdminService/InvalidateQuestionCache";
              readonly requestType: "InvalidateQuestionCacheRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#listAdminAccounts}.
             * @param error Error, if any
             * @param [response] ListAdminAccountsResponse
             */
            type ListAdminAccountsCallback = (error: (Error|null), response?: oj.biz.ListAdminAccountsResponse) => void;

            /** Calls ListAdminAccounts. */
            type ListAdminAccounts = {
              (request: oj.biz.IListAdminAccountsRequest, callback: oj.biz.AdminService.ListAdminAccountsCallback): void;
              (request: oj.biz.IListAdminAccountsRequest): Promise<oj.biz.ListAdminAccountsResponse>;
              readonly name: "ListAdminAccounts";
              readonly path: "/oj.biz.AdminService/ListAdminAccounts";
              readonly requestType: "ListAdminAccountsRequest";
              readonly responseType: "ListAdminAccountsResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#getAdminAccount}.
             * @param error Error, if any
             * @param [response] AdminAccountResponse
             */
            type GetAdminAccountCallback = (error: (Error|null), response?: oj.biz.AdminAccountResponse) => void;

            /** Calls GetAdminAccount. */
            type GetAdminAccount = {
              (request: oj.biz.IAdminAccountIdRequest, callback: oj.biz.AdminService.GetAdminAccountCallback): void;
              (request: oj.biz.IAdminAccountIdRequest): Promise<oj.biz.AdminAccountResponse>;
              readonly name: "GetAdminAccount";
              readonly path: "/oj.biz.AdminService/GetAdminAccount";
              readonly requestType: "AdminAccountIdRequest";
              readonly responseType: "AdminAccountResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#createAdminAccount}.
             * @param error Error, if any
             * @param [response] AdminAccountResponse
             */
            type CreateAdminAccountCallback = (error: (Error|null), response?: oj.biz.AdminAccountResponse) => void;

            /** Calls CreateAdminAccount. */
            type CreateAdminAccount = {
              (request: oj.biz.ISaveAdminAccountRequest, callback: oj.biz.AdminService.CreateAdminAccountCallback): void;
              (request: oj.biz.ISaveAdminAccountRequest): Promise<oj.biz.AdminAccountResponse>;
              readonly name: "CreateAdminAccount";
              readonly path: "/oj.biz.AdminService/CreateAdminAccount";
              readonly requestType: "SaveAdminAccountRequest";
              readonly responseType: "AdminAccountResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#updateAdminAccount}.
             * @param error Error, if any
             * @param [response] AdminAccountResponse
             */
            type UpdateAdminAccountCallback = (error: (Error|null), response?: oj.biz.AdminAccountResponse) => void;

            /** Calls UpdateAdminAccount. */
            type UpdateAdminAccount = {
              (request: oj.biz.ISaveAdminAccountRequest, callback: oj.biz.AdminService.UpdateAdminAccountCallback): void;
              (request: oj.biz.ISaveAdminAccountRequest): Promise<oj.biz.AdminAccountResponse>;
              readonly name: "UpdateAdminAccount";
              readonly path: "/oj.biz.AdminService/UpdateAdminAccount";
              readonly requestType: "SaveAdminAccountRequest";
              readonly responseType: "AdminAccountResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#deleteAdminAccount}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type DeleteAdminAccountCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls DeleteAdminAccount. */
            type DeleteAdminAccount = {
              (request: oj.biz.IAdminAccountIdRequest, callback: oj.biz.AdminService.DeleteAdminAccountCallback): void;
              (request: oj.biz.IAdminAccountIdRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "DeleteAdminAccount";
              readonly path: "/oj.biz.AdminService/DeleteAdminAccount";
              readonly requestType: "AdminAccountIdRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#getCacheMetrics}.
             * @param error Error, if any
             * @param [response] GetCacheMetricsResponse
             */
            type GetCacheMetricsCallback = (error: (Error|null), response?: oj.biz.GetCacheMetricsResponse) => void;

            /** Calls GetCacheMetrics. */
            type GetCacheMetrics = {
              (request: oj.common.IEmptyRequest, callback: oj.biz.AdminService.GetCacheMetricsCallback): void;
              (request: oj.common.IEmptyRequest): Promise<oj.biz.GetCacheMetricsResponse>;
              readonly name: "GetCacheMetrics";
              readonly path: "/oj.biz.AdminService/GetCacheMetrics";
              readonly requestType: "oj.common.EmptyRequest";
              readonly responseType: "GetCacheMetricsResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.AdminService#getAuditLogs}.
             * @param error Error, if any
             * @param [response] GetAuditLogsResponse
             */
            type GetAuditLogsCallback = (error: (Error|null), response?: oj.biz.GetAuditLogsResponse) => void;

            /** Calls GetAuditLogs. */
            type GetAuditLogs = {
              (request: oj.biz.IGetAuditLogsRequest, callback: oj.biz.AdminService.GetAuditLogsCallback): void;
              (request: oj.biz.IGetAuditLogsRequest): Promise<oj.biz.GetAuditLogsResponse>;
              readonly name: "GetAuditLogs";
              readonly path: "/oj.biz.AdminService/GetAuditLogs";
              readonly requestType: "GetAuditLogsRequest";
              readonly responseType: "GetAuditLogsResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };
        }

        /**
         * Properties of a CreateSubmissionRequest.
         * @deprecated Use oj.biz.CreateSubmissionRequest.$Properties instead.
         */
        interface ICreateSubmissionRequest extends oj.biz.CreateSubmissionRequest.$Properties {
        }

        /** Represents a CreateSubmissionRequest. */
        class CreateSubmissionRequest {

            /**
             * Constructs a new CreateSubmissionRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.CreateSubmissionRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CreateSubmissionRequest questionId. */
            questionId: string;

            /** CreateSubmissionRequest code. */
            code: string;

            /** CreateSubmissionRequest language. */
            language: string;

            /**
             * Creates a new CreateSubmissionRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CreateSubmissionRequest instance
             */
            static create(properties: oj.biz.CreateSubmissionRequest.$Shape): oj.biz.CreateSubmissionRequest & oj.biz.CreateSubmissionRequest.$Shape;
            static create(properties?: oj.biz.CreateSubmissionRequest.$Properties): oj.biz.CreateSubmissionRequest;

            /**
             * Encodes the specified CreateSubmissionRequest message. Does not implicitly {@link oj.biz.CreateSubmissionRequest.verify|verify} messages.
             * @param message CreateSubmissionRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.CreateSubmissionRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CreateSubmissionRequest message, length delimited. Does not implicitly {@link oj.biz.CreateSubmissionRequest.verify|verify} messages.
             * @param message CreateSubmissionRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.CreateSubmissionRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CreateSubmissionRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.CreateSubmissionRequest & oj.biz.CreateSubmissionRequest.$Shape} CreateSubmissionRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.CreateSubmissionRequest & oj.biz.CreateSubmissionRequest.$Shape;

            /**
             * Decodes a CreateSubmissionRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.CreateSubmissionRequest & oj.biz.CreateSubmissionRequest.$Shape} CreateSubmissionRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.CreateSubmissionRequest & oj.biz.CreateSubmissionRequest.$Shape;

            /**
             * Verifies a CreateSubmissionRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CreateSubmissionRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CreateSubmissionRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.CreateSubmissionRequest;

            /**
             * Creates a plain object from a CreateSubmissionRequest message. Also converts values to other types if specified.
             * @param message CreateSubmissionRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.CreateSubmissionRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CreateSubmissionRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CreateSubmissionRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CreateSubmissionRequest {

            /** Properties of a CreateSubmissionRequest. */
            interface $Properties {

                /** CreateSubmissionRequest questionId */
                questionId?: (string|null);

                /** CreateSubmissionRequest code */
                code?: (string|null);

                /** CreateSubmissionRequest language */
                language?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CreateSubmissionRequest. */
            type $Shape = oj.biz.CreateSubmissionRequest.$Properties;
        }

        /**
         * Properties of a CreateSubmissionResponse.
         * @deprecated Use oj.biz.CreateSubmissionResponse.$Properties instead.
         */
        interface ICreateSubmissionResponse extends oj.biz.CreateSubmissionResponse.$Properties {
        }

        /** Represents a CreateSubmissionResponse. */
        class CreateSubmissionResponse {

            /**
             * Constructs a new CreateSubmissionResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.CreateSubmissionResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CreateSubmissionResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** CreateSubmissionResponse submission. */
            submission?: (oj.common.Submission.$Properties|null);

            /**
             * Creates a new CreateSubmissionResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CreateSubmissionResponse instance
             */
            static create(properties: oj.biz.CreateSubmissionResponse.$Shape): oj.biz.CreateSubmissionResponse & oj.biz.CreateSubmissionResponse.$Shape;
            static create(properties?: oj.biz.CreateSubmissionResponse.$Properties): oj.biz.CreateSubmissionResponse;

            /**
             * Encodes the specified CreateSubmissionResponse message. Does not implicitly {@link oj.biz.CreateSubmissionResponse.verify|verify} messages.
             * @param message CreateSubmissionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.CreateSubmissionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CreateSubmissionResponse message, length delimited. Does not implicitly {@link oj.biz.CreateSubmissionResponse.verify|verify} messages.
             * @param message CreateSubmissionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.CreateSubmissionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CreateSubmissionResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.CreateSubmissionResponse & oj.biz.CreateSubmissionResponse.$Shape} CreateSubmissionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.CreateSubmissionResponse & oj.biz.CreateSubmissionResponse.$Shape;

            /**
             * Decodes a CreateSubmissionResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.CreateSubmissionResponse & oj.biz.CreateSubmissionResponse.$Shape} CreateSubmissionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.CreateSubmissionResponse & oj.biz.CreateSubmissionResponse.$Shape;

            /**
             * Verifies a CreateSubmissionResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CreateSubmissionResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CreateSubmissionResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.CreateSubmissionResponse;

            /**
             * Creates a plain object from a CreateSubmissionResponse message. Also converts values to other types if specified.
             * @param message CreateSubmissionResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.CreateSubmissionResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CreateSubmissionResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CreateSubmissionResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CreateSubmissionResponse {

            /** Properties of a CreateSubmissionResponse. */
            interface $Properties {

                /** CreateSubmissionResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** CreateSubmissionResponse submission */
                submission?: (oj.common.Submission.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CreateSubmissionResponse. */
            type $Shape = oj.biz.CreateSubmissionResponse.$Properties;
        }

        /**
         * Properties of a CreateCustomTestRequest.
         * @deprecated Use oj.biz.CreateCustomTestRequest.$Properties instead.
         */
        interface ICreateCustomTestRequest extends oj.biz.CreateCustomTestRequest.$Properties {
        }

        /** Represents a CreateCustomTestRequest. */
        class CreateCustomTestRequest {

            /**
             * Constructs a new CreateCustomTestRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.CreateCustomTestRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CreateCustomTestRequest questionId. */
            questionId: string;

            /** CreateCustomTestRequest code. */
            code: string;

            /** CreateCustomTestRequest language. */
            language: string;

            /** CreateCustomTestRequest customInput. */
            customInput: string;

            /**
             * Creates a new CreateCustomTestRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CreateCustomTestRequest instance
             */
            static create(properties: oj.biz.CreateCustomTestRequest.$Shape): oj.biz.CreateCustomTestRequest & oj.biz.CreateCustomTestRequest.$Shape;
            static create(properties?: oj.biz.CreateCustomTestRequest.$Properties): oj.biz.CreateCustomTestRequest;

            /**
             * Encodes the specified CreateCustomTestRequest message. Does not implicitly {@link oj.biz.CreateCustomTestRequest.verify|verify} messages.
             * @param message CreateCustomTestRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.CreateCustomTestRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CreateCustomTestRequest message, length delimited. Does not implicitly {@link oj.biz.CreateCustomTestRequest.verify|verify} messages.
             * @param message CreateCustomTestRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.CreateCustomTestRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CreateCustomTestRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.CreateCustomTestRequest & oj.biz.CreateCustomTestRequest.$Shape} CreateCustomTestRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.CreateCustomTestRequest & oj.biz.CreateCustomTestRequest.$Shape;

            /**
             * Decodes a CreateCustomTestRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.CreateCustomTestRequest & oj.biz.CreateCustomTestRequest.$Shape} CreateCustomTestRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.CreateCustomTestRequest & oj.biz.CreateCustomTestRequest.$Shape;

            /**
             * Verifies a CreateCustomTestRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CreateCustomTestRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CreateCustomTestRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.CreateCustomTestRequest;

            /**
             * Creates a plain object from a CreateCustomTestRequest message. Also converts values to other types if specified.
             * @param message CreateCustomTestRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.CreateCustomTestRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CreateCustomTestRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CreateCustomTestRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CreateCustomTestRequest {

            /** Properties of a CreateCustomTestRequest. */
            interface $Properties {

                /** CreateCustomTestRequest questionId */
                questionId?: (string|null);

                /** CreateCustomTestRequest code */
                code?: (string|null);

                /** CreateCustomTestRequest language */
                language?: (string|null);

                /** CreateCustomTestRequest customInput */
                customInput?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CreateCustomTestRequest. */
            type $Shape = oj.biz.CreateCustomTestRequest.$Properties;
        }

        /**
         * Properties of a CreateCustomTestResponse.
         * @deprecated Use oj.biz.CreateCustomTestResponse.$Properties instead.
         */
        interface ICreateCustomTestResponse extends oj.biz.CreateCustomTestResponse.$Properties {
        }

        /** Represents a CreateCustomTestResponse. */
        class CreateCustomTestResponse {

            /**
             * Constructs a new CreateCustomTestResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.CreateCustomTestResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CreateCustomTestResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** CreateCustomTestResponse taskId. */
            taskId: string;

            /** CreateCustomTestResponse submissionStatus. */
            submissionStatus: oj.common.SubmissionStatus;

            /**
             * Creates a new CreateCustomTestResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CreateCustomTestResponse instance
             */
            static create(properties: oj.biz.CreateCustomTestResponse.$Shape): oj.biz.CreateCustomTestResponse & oj.biz.CreateCustomTestResponse.$Shape;
            static create(properties?: oj.biz.CreateCustomTestResponse.$Properties): oj.biz.CreateCustomTestResponse;

            /**
             * Encodes the specified CreateCustomTestResponse message. Does not implicitly {@link oj.biz.CreateCustomTestResponse.verify|verify} messages.
             * @param message CreateCustomTestResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.CreateCustomTestResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CreateCustomTestResponse message, length delimited. Does not implicitly {@link oj.biz.CreateCustomTestResponse.verify|verify} messages.
             * @param message CreateCustomTestResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.CreateCustomTestResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CreateCustomTestResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.CreateCustomTestResponse & oj.biz.CreateCustomTestResponse.$Shape} CreateCustomTestResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.CreateCustomTestResponse & oj.biz.CreateCustomTestResponse.$Shape;

            /**
             * Decodes a CreateCustomTestResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.CreateCustomTestResponse & oj.biz.CreateCustomTestResponse.$Shape} CreateCustomTestResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.CreateCustomTestResponse & oj.biz.CreateCustomTestResponse.$Shape;

            /**
             * Verifies a CreateCustomTestResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CreateCustomTestResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CreateCustomTestResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.CreateCustomTestResponse;

            /**
             * Creates a plain object from a CreateCustomTestResponse message. Also converts values to other types if specified.
             * @param message CreateCustomTestResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.CreateCustomTestResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CreateCustomTestResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CreateCustomTestResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CreateCustomTestResponse {

            /** Properties of a CreateCustomTestResponse. */
            interface $Properties {

                /** CreateCustomTestResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** CreateCustomTestResponse taskId */
                taskId?: (string|null);

                /** CreateCustomTestResponse submissionStatus */
                submissionStatus?: (oj.common.SubmissionStatus|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CreateCustomTestResponse. */
            type $Shape = oj.biz.CreateCustomTestResponse.$Properties;
        }

        /**
         * Properties of a GetSubmissionRequest.
         * @deprecated Use oj.biz.GetSubmissionRequest.$Properties instead.
         */
        interface IGetSubmissionRequest extends oj.biz.GetSubmissionRequest.$Properties {
        }

        /** Represents a GetSubmissionRequest. */
        class GetSubmissionRequest {

            /**
             * Constructs a new GetSubmissionRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetSubmissionRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetSubmissionRequest submissionId. */
            submissionId: (Long|string);

            /**
             * Creates a new GetSubmissionRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetSubmissionRequest instance
             */
            static create(properties: oj.biz.GetSubmissionRequest.$Shape): oj.biz.GetSubmissionRequest & oj.biz.GetSubmissionRequest.$Shape;
            static create(properties?: oj.biz.GetSubmissionRequest.$Properties): oj.biz.GetSubmissionRequest;

            /**
             * Encodes the specified GetSubmissionRequest message. Does not implicitly {@link oj.biz.GetSubmissionRequest.verify|verify} messages.
             * @param message GetSubmissionRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetSubmissionRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetSubmissionRequest message, length delimited. Does not implicitly {@link oj.biz.GetSubmissionRequest.verify|verify} messages.
             * @param message GetSubmissionRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetSubmissionRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetSubmissionRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetSubmissionRequest & oj.biz.GetSubmissionRequest.$Shape} GetSubmissionRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetSubmissionRequest & oj.biz.GetSubmissionRequest.$Shape;

            /**
             * Decodes a GetSubmissionRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetSubmissionRequest & oj.biz.GetSubmissionRequest.$Shape} GetSubmissionRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetSubmissionRequest & oj.biz.GetSubmissionRequest.$Shape;

            /**
             * Verifies a GetSubmissionRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetSubmissionRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetSubmissionRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetSubmissionRequest;

            /**
             * Creates a plain object from a GetSubmissionRequest message. Also converts values to other types if specified.
             * @param message GetSubmissionRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetSubmissionRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetSubmissionRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetSubmissionRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetSubmissionRequest {

            /** Properties of a GetSubmissionRequest. */
            interface $Properties {

                /** GetSubmissionRequest submissionId */
                submissionId?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetSubmissionRequest. */
            type $Shape = oj.biz.GetSubmissionRequest.$Properties;
        }

        /**
         * Properties of a GetSubmissionResponse.
         * @deprecated Use oj.biz.GetSubmissionResponse.$Properties instead.
         */
        interface IGetSubmissionResponse extends oj.biz.GetSubmissionResponse.$Properties {
        }

        /** Represents a GetSubmissionResponse. */
        class GetSubmissionResponse {

            /**
             * Constructs a new GetSubmissionResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetSubmissionResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetSubmissionResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetSubmissionResponse submission. */
            submission?: (oj.common.Submission.$Properties|null);

            /**
             * Creates a new GetSubmissionResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetSubmissionResponse instance
             */
            static create(properties: oj.biz.GetSubmissionResponse.$Shape): oj.biz.GetSubmissionResponse & oj.biz.GetSubmissionResponse.$Shape;
            static create(properties?: oj.biz.GetSubmissionResponse.$Properties): oj.biz.GetSubmissionResponse;

            /**
             * Encodes the specified GetSubmissionResponse message. Does not implicitly {@link oj.biz.GetSubmissionResponse.verify|verify} messages.
             * @param message GetSubmissionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetSubmissionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetSubmissionResponse message, length delimited. Does not implicitly {@link oj.biz.GetSubmissionResponse.verify|verify} messages.
             * @param message GetSubmissionResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetSubmissionResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetSubmissionResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetSubmissionResponse & oj.biz.GetSubmissionResponse.$Shape} GetSubmissionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetSubmissionResponse & oj.biz.GetSubmissionResponse.$Shape;

            /**
             * Decodes a GetSubmissionResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetSubmissionResponse & oj.biz.GetSubmissionResponse.$Shape} GetSubmissionResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetSubmissionResponse & oj.biz.GetSubmissionResponse.$Shape;

            /**
             * Verifies a GetSubmissionResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetSubmissionResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetSubmissionResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetSubmissionResponse;

            /**
             * Creates a plain object from a GetSubmissionResponse message. Also converts values to other types if specified.
             * @param message GetSubmissionResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetSubmissionResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetSubmissionResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetSubmissionResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetSubmissionResponse {

            /** Properties of a GetSubmissionResponse. */
            interface $Properties {

                /** GetSubmissionResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetSubmissionResponse submission */
                submission?: (oj.common.Submission.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetSubmissionResponse. */
            type $Shape = oj.biz.GetSubmissionResponse.$Properties;
        }

        /**
         * Properties of a GetCustomTestRequest.
         * @deprecated Use oj.biz.GetCustomTestRequest.$Properties instead.
         */
        interface IGetCustomTestRequest extends oj.biz.GetCustomTestRequest.$Properties {
        }

        /** Represents a GetCustomTestRequest. */
        class GetCustomTestRequest {

            /**
             * Constructs a new GetCustomTestRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetCustomTestRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetCustomTestRequest taskId. */
            taskId: string;

            /**
             * Creates a new GetCustomTestRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetCustomTestRequest instance
             */
            static create(properties: oj.biz.GetCustomTestRequest.$Shape): oj.biz.GetCustomTestRequest & oj.biz.GetCustomTestRequest.$Shape;
            static create(properties?: oj.biz.GetCustomTestRequest.$Properties): oj.biz.GetCustomTestRequest;

            /**
             * Encodes the specified GetCustomTestRequest message. Does not implicitly {@link oj.biz.GetCustomTestRequest.verify|verify} messages.
             * @param message GetCustomTestRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetCustomTestRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetCustomTestRequest message, length delimited. Does not implicitly {@link oj.biz.GetCustomTestRequest.verify|verify} messages.
             * @param message GetCustomTestRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetCustomTestRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetCustomTestRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetCustomTestRequest & oj.biz.GetCustomTestRequest.$Shape} GetCustomTestRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetCustomTestRequest & oj.biz.GetCustomTestRequest.$Shape;

            /**
             * Decodes a GetCustomTestRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetCustomTestRequest & oj.biz.GetCustomTestRequest.$Shape} GetCustomTestRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetCustomTestRequest & oj.biz.GetCustomTestRequest.$Shape;

            /**
             * Verifies a GetCustomTestRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetCustomTestRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetCustomTestRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetCustomTestRequest;

            /**
             * Creates a plain object from a GetCustomTestRequest message. Also converts values to other types if specified.
             * @param message GetCustomTestRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetCustomTestRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetCustomTestRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetCustomTestRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetCustomTestRequest {

            /** Properties of a GetCustomTestRequest. */
            interface $Properties {

                /** GetCustomTestRequest taskId */
                taskId?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetCustomTestRequest. */
            type $Shape = oj.biz.GetCustomTestRequest.$Properties;
        }

        /**
         * Properties of a GetCustomTestResponse.
         * @deprecated Use oj.biz.GetCustomTestResponse.$Properties instead.
         */
        interface IGetCustomTestResponse extends oj.biz.GetCustomTestResponse.$Properties {
        }

        /** Represents a GetCustomTestResponse. */
        class GetCustomTestResponse {

            /**
             * Constructs a new GetCustomTestResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.GetCustomTestResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** GetCustomTestResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** GetCustomTestResponse taskId. */
            taskId: string;

            /** GetCustomTestResponse submissionStatus. */
            submissionStatus: oj.common.SubmissionStatus;

            /** GetCustomTestResponse result. */
            result?: (oj.common.JudgeResult.$Properties|null);

            /**
             * Creates a new GetCustomTestResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns GetCustomTestResponse instance
             */
            static create(properties: oj.biz.GetCustomTestResponse.$Shape): oj.biz.GetCustomTestResponse & oj.biz.GetCustomTestResponse.$Shape;
            static create(properties?: oj.biz.GetCustomTestResponse.$Properties): oj.biz.GetCustomTestResponse;

            /**
             * Encodes the specified GetCustomTestResponse message. Does not implicitly {@link oj.biz.GetCustomTestResponse.verify|verify} messages.
             * @param message GetCustomTestResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.GetCustomTestResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified GetCustomTestResponse message, length delimited. Does not implicitly {@link oj.biz.GetCustomTestResponse.verify|verify} messages.
             * @param message GetCustomTestResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.GetCustomTestResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a GetCustomTestResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.GetCustomTestResponse & oj.biz.GetCustomTestResponse.$Shape} GetCustomTestResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.GetCustomTestResponse & oj.biz.GetCustomTestResponse.$Shape;

            /**
             * Decodes a GetCustomTestResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.GetCustomTestResponse & oj.biz.GetCustomTestResponse.$Shape} GetCustomTestResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.GetCustomTestResponse & oj.biz.GetCustomTestResponse.$Shape;

            /**
             * Verifies a GetCustomTestResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a GetCustomTestResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns GetCustomTestResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.GetCustomTestResponse;

            /**
             * Creates a plain object from a GetCustomTestResponse message. Also converts values to other types if specified.
             * @param message GetCustomTestResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.GetCustomTestResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this GetCustomTestResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for GetCustomTestResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace GetCustomTestResponse {

            /** Properties of a GetCustomTestResponse. */
            interface $Properties {

                /** GetCustomTestResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** GetCustomTestResponse taskId */
                taskId?: (string|null);

                /** GetCustomTestResponse submissionStatus */
                submissionStatus?: (oj.common.SubmissionStatus|null);

                /** GetCustomTestResponse result */
                result?: (oj.common.JudgeResult.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a GetCustomTestResponse. */
            type $Shape = oj.biz.GetCustomTestResponse.$Properties;
        }

        /**
         * Properties of a ListSubmissionsRequest.
         * @deprecated Use oj.biz.ListSubmissionsRequest.$Properties instead.
         */
        interface IListSubmissionsRequest extends oj.biz.ListSubmissionsRequest.$Properties {
        }

        /** Represents a ListSubmissionsRequest. */
        class ListSubmissionsRequest {

            /**
             * Constructs a new ListSubmissionsRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListSubmissionsRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListSubmissionsRequest page. */
            page?: (oj.common.PageRequest.$Properties|null);

            /** ListSubmissionsRequest questionId. */
            questionId: string;

            /** ListSubmissionsRequest submissionStatus. */
            submissionStatus: oj.common.SubmissionStatus;

            /**
             * Creates a new ListSubmissionsRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListSubmissionsRequest instance
             */
            static create(properties: oj.biz.ListSubmissionsRequest.$Shape): oj.biz.ListSubmissionsRequest & oj.biz.ListSubmissionsRequest.$Shape;
            static create(properties?: oj.biz.ListSubmissionsRequest.$Properties): oj.biz.ListSubmissionsRequest;

            /**
             * Encodes the specified ListSubmissionsRequest message. Does not implicitly {@link oj.biz.ListSubmissionsRequest.verify|verify} messages.
             * @param message ListSubmissionsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListSubmissionsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListSubmissionsRequest message, length delimited. Does not implicitly {@link oj.biz.ListSubmissionsRequest.verify|verify} messages.
             * @param message ListSubmissionsRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListSubmissionsRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListSubmissionsRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListSubmissionsRequest & oj.biz.ListSubmissionsRequest.$Shape} ListSubmissionsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListSubmissionsRequest & oj.biz.ListSubmissionsRequest.$Shape;

            /**
             * Decodes a ListSubmissionsRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListSubmissionsRequest & oj.biz.ListSubmissionsRequest.$Shape} ListSubmissionsRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListSubmissionsRequest & oj.biz.ListSubmissionsRequest.$Shape;

            /**
             * Verifies a ListSubmissionsRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListSubmissionsRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListSubmissionsRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListSubmissionsRequest;

            /**
             * Creates a plain object from a ListSubmissionsRequest message. Also converts values to other types if specified.
             * @param message ListSubmissionsRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListSubmissionsRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListSubmissionsRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListSubmissionsRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListSubmissionsRequest {

            /** Properties of a ListSubmissionsRequest. */
            interface $Properties {

                /** ListSubmissionsRequest page */
                page?: (oj.common.PageRequest.$Properties|null);

                /** ListSubmissionsRequest questionId */
                questionId?: (string|null);

                /** ListSubmissionsRequest submissionStatus */
                submissionStatus?: (oj.common.SubmissionStatus|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListSubmissionsRequest. */
            type $Shape = oj.biz.ListSubmissionsRequest.$Properties;
        }

        /**
         * Properties of a ListSubmissionsResponse.
         * @deprecated Use oj.biz.ListSubmissionsResponse.$Properties instead.
         */
        interface IListSubmissionsResponse extends oj.biz.ListSubmissionsResponse.$Properties {
        }

        /** Represents a ListSubmissionsResponse. */
        class ListSubmissionsResponse {

            /**
             * Constructs a new ListSubmissionsResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ListSubmissionsResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ListSubmissionsResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** ListSubmissionsResponse page. */
            page?: (oj.common.PageResponse.$Properties|null);

            /** ListSubmissionsResponse submissions. */
            submissions: oj.common.Submission.$Properties[];

            /**
             * Creates a new ListSubmissionsResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ListSubmissionsResponse instance
             */
            static create(properties: oj.biz.ListSubmissionsResponse.$Shape): oj.biz.ListSubmissionsResponse & oj.biz.ListSubmissionsResponse.$Shape;
            static create(properties?: oj.biz.ListSubmissionsResponse.$Properties): oj.biz.ListSubmissionsResponse;

            /**
             * Encodes the specified ListSubmissionsResponse message. Does not implicitly {@link oj.biz.ListSubmissionsResponse.verify|verify} messages.
             * @param message ListSubmissionsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ListSubmissionsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ListSubmissionsResponse message, length delimited. Does not implicitly {@link oj.biz.ListSubmissionsResponse.verify|verify} messages.
             * @param message ListSubmissionsResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ListSubmissionsResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ListSubmissionsResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ListSubmissionsResponse & oj.biz.ListSubmissionsResponse.$Shape} ListSubmissionsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ListSubmissionsResponse & oj.biz.ListSubmissionsResponse.$Shape;

            /**
             * Decodes a ListSubmissionsResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ListSubmissionsResponse & oj.biz.ListSubmissionsResponse.$Shape} ListSubmissionsResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ListSubmissionsResponse & oj.biz.ListSubmissionsResponse.$Shape;

            /**
             * Verifies a ListSubmissionsResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ListSubmissionsResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ListSubmissionsResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ListSubmissionsResponse;

            /**
             * Creates a plain object from a ListSubmissionsResponse message. Also converts values to other types if specified.
             * @param message ListSubmissionsResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ListSubmissionsResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ListSubmissionsResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ListSubmissionsResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ListSubmissionsResponse {

            /** Properties of a ListSubmissionsResponse. */
            interface $Properties {

                /** ListSubmissionsResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** ListSubmissionsResponse page */
                page?: (oj.common.PageResponse.$Properties|null);

                /** ListSubmissionsResponse submissions */
                submissions?: (oj.common.Submission.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ListSubmissionsResponse. */
            type $Shape = oj.biz.ListSubmissionsResponse.$Properties;
        }

        /**
         * Properties of an UpdateJudgeResultRequest.
         * @deprecated Use oj.biz.UpdateJudgeResultRequest.$Properties instead.
         */
        interface IUpdateJudgeResultRequest extends oj.biz.UpdateJudgeResultRequest.$Properties {
        }

        /** Represents an UpdateJudgeResultRequest. */
        class UpdateJudgeResultRequest {

            /**
             * Constructs a new UpdateJudgeResultRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.UpdateJudgeResultRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** UpdateJudgeResultRequest messageId. */
            messageId: string;

            /** UpdateJudgeResultRequest submissionId. */
            submissionId?: ((Long|string)|null);

            /** UpdateJudgeResultRequest customTaskId. */
            customTaskId?: (string|null);

            /** UpdateJudgeResultRequest resultPayload. */
            resultPayload: Uint8Array;

            /** UpdateJudgeResultRequest serviceAuth. */
            serviceAuth?: (oj.biz.ServiceAuthentication.$Properties|null);

            /** UpdateJudgeResultRequest taskId. */
            taskId?: ("submissionId"|"customTaskId");

            /**
             * Creates a new UpdateJudgeResultRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UpdateJudgeResultRequest instance
             */
            static create(properties: oj.biz.UpdateJudgeResultRequest.$Shape): oj.biz.UpdateJudgeResultRequest & oj.biz.UpdateJudgeResultRequest.$Shape;
            static create(properties?: oj.biz.UpdateJudgeResultRequest.$Properties): oj.biz.UpdateJudgeResultRequest;

            /**
             * Encodes the specified UpdateJudgeResultRequest message. Does not implicitly {@link oj.biz.UpdateJudgeResultRequest.verify|verify} messages.
             * @param message UpdateJudgeResultRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.UpdateJudgeResultRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UpdateJudgeResultRequest message, length delimited. Does not implicitly {@link oj.biz.UpdateJudgeResultRequest.verify|verify} messages.
             * @param message UpdateJudgeResultRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.UpdateJudgeResultRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an UpdateJudgeResultRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.UpdateJudgeResultRequest & oj.biz.UpdateJudgeResultRequest.$Shape} UpdateJudgeResultRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.UpdateJudgeResultRequest & oj.biz.UpdateJudgeResultRequest.$Shape;

            /**
             * Decodes an UpdateJudgeResultRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.UpdateJudgeResultRequest & oj.biz.UpdateJudgeResultRequest.$Shape} UpdateJudgeResultRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.UpdateJudgeResultRequest & oj.biz.UpdateJudgeResultRequest.$Shape;

            /**
             * Verifies an UpdateJudgeResultRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an UpdateJudgeResultRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UpdateJudgeResultRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.UpdateJudgeResultRequest;

            /**
             * Creates a plain object from an UpdateJudgeResultRequest message. Also converts values to other types if specified.
             * @param message UpdateJudgeResultRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.UpdateJudgeResultRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UpdateJudgeResultRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UpdateJudgeResultRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UpdateJudgeResultRequest {

            /** Properties of an UpdateJudgeResultRequest. */
            interface $Properties {

                /** UpdateJudgeResultRequest messageId */
                messageId?: (string|null);

                /** UpdateJudgeResultRequest submissionId */
                submissionId?: ((Long|string)|null);

                /** UpdateJudgeResultRequest customTaskId */
                customTaskId?: (string|null);

                /** UpdateJudgeResultRequest resultPayload */
                resultPayload?: (Uint8Array|null);

                /** UpdateJudgeResultRequest serviceAuth */
                serviceAuth?: (oj.biz.ServiceAuthentication.$Properties|null);

                /** UpdateJudgeResultRequest taskId */
                taskId?: ("submissionId"|"customTaskId");

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Narrowed shape of an UpdateJudgeResultRequest. */
            type $Shape = {
              messageId?: string|null;
              submissionId?: (Long|string)|null;
              customTaskId?: string|null;
              resultPayload?: Uint8Array|null;
              serviceAuth?: oj.biz.ServiceAuthentication.$Shape|null;
              $unknowns?: Uint8Array[];
            } & (
              ({ taskId?: undefined; submissionId?: null; customTaskId?: null }|{ taskId?: "submissionId"; submissionId: (Long|string); customTaskId?: null }|{ taskId?: "customTaskId"; submissionId?: null; customTaskId: string })
            );
        }

        /**
         * Properties of a ServiceAuthentication.
         * @deprecated Use oj.biz.ServiceAuthentication.$Properties instead.
         */
        interface IServiceAuthentication extends oj.biz.ServiceAuthentication.$Properties {
        }

        /** Represents a ServiceAuthentication. */
        class ServiceAuthentication {

            /**
             * Constructs a new ServiceAuthentication.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.ServiceAuthentication.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** ServiceAuthentication keyId. */
            keyId: string;

            /** ServiceAuthentication timestamp. */
            timestamp: (Long|string);

            /** ServiceAuthentication nonce. */
            nonce: string;

            /** ServiceAuthentication hmacSha256. */
            hmacSha256: Uint8Array;

            /**
             * Creates a new ServiceAuthentication instance using the specified properties.
             * @param [properties] Properties to set
             * @returns ServiceAuthentication instance
             */
            static create(properties: oj.biz.ServiceAuthentication.$Shape): oj.biz.ServiceAuthentication & oj.biz.ServiceAuthentication.$Shape;
            static create(properties?: oj.biz.ServiceAuthentication.$Properties): oj.biz.ServiceAuthentication;

            /**
             * Encodes the specified ServiceAuthentication message. Does not implicitly {@link oj.biz.ServiceAuthentication.verify|verify} messages.
             * @param message ServiceAuthentication message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.ServiceAuthentication.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified ServiceAuthentication message, length delimited. Does not implicitly {@link oj.biz.ServiceAuthentication.verify|verify} messages.
             * @param message ServiceAuthentication message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.ServiceAuthentication.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a ServiceAuthentication message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.ServiceAuthentication & oj.biz.ServiceAuthentication.$Shape} ServiceAuthentication
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.ServiceAuthentication & oj.biz.ServiceAuthentication.$Shape;

            /**
             * Decodes a ServiceAuthentication message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.ServiceAuthentication & oj.biz.ServiceAuthentication.$Shape} ServiceAuthentication
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.ServiceAuthentication & oj.biz.ServiceAuthentication.$Shape;

            /**
             * Verifies a ServiceAuthentication message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a ServiceAuthentication message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns ServiceAuthentication
             */
            static fromObject(object: { [k: string]: any }): oj.biz.ServiceAuthentication;

            /**
             * Creates a plain object from a ServiceAuthentication message. Also converts values to other types if specified.
             * @param message ServiceAuthentication
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.ServiceAuthentication, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this ServiceAuthentication to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for ServiceAuthentication
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace ServiceAuthentication {

            /** Properties of a ServiceAuthentication. */
            interface $Properties {

                /** ServiceAuthentication keyId */
                keyId?: (string|null);

                /** ServiceAuthentication timestamp */
                timestamp?: ((Long|string)|null);

                /** ServiceAuthentication nonce */
                nonce?: (string|null);

                /** ServiceAuthentication hmacSha256 */
                hmacSha256?: (Uint8Array|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a ServiceAuthentication. */
            type $Shape = oj.biz.ServiceAuthentication.$Properties;
        }

        /** JudgeResultPersistenceOutcome enum. */
        enum JudgeResultPersistenceOutcome {

            /** JUDGE_RESULT_PERSISTENCE_OUTCOME_UNSPECIFIED value */
            JUDGE_RESULT_PERSISTENCE_OUTCOME_UNSPECIFIED = 0,

            /** JUDGE_RESULT_PERSISTENCE_OUTCOME_PERSISTED value */
            JUDGE_RESULT_PERSISTENCE_OUTCOME_PERSISTED = 1,

            /** JUDGE_RESULT_PERSISTENCE_OUTCOME_IDEMPOTENT value */
            JUDGE_RESULT_PERSISTENCE_OUTCOME_IDEMPOTENT = 2,

            /** JUDGE_RESULT_PERSISTENCE_OUTCOME_RETRYABLE_FAILURE value */
            JUDGE_RESULT_PERSISTENCE_OUTCOME_RETRYABLE_FAILURE = 3,

            /** JUDGE_RESULT_PERSISTENCE_OUTCOME_REJECTED value */
            JUDGE_RESULT_PERSISTENCE_OUTCOME_REJECTED = 4
        }

        /**
         * Properties of an UpdateJudgeResultResponse.
         * @deprecated Use oj.biz.UpdateJudgeResultResponse.$Properties instead.
         */
        interface IUpdateJudgeResultResponse extends oj.biz.UpdateJudgeResultResponse.$Properties {
        }

        /** Represents an UpdateJudgeResultResponse. */
        class UpdateJudgeResultResponse {

            /**
             * Constructs a new UpdateJudgeResultResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.UpdateJudgeResultResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** UpdateJudgeResultResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /** UpdateJudgeResultResponse outcome. */
            outcome: oj.biz.JudgeResultPersistenceOutcome;

            /** UpdateJudgeResultResponse retryAfterMs. */
            retryAfterMs: number;

            /**
             * Creates a new UpdateJudgeResultResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UpdateJudgeResultResponse instance
             */
            static create(properties: oj.biz.UpdateJudgeResultResponse.$Shape): oj.biz.UpdateJudgeResultResponse & oj.biz.UpdateJudgeResultResponse.$Shape;
            static create(properties?: oj.biz.UpdateJudgeResultResponse.$Properties): oj.biz.UpdateJudgeResultResponse;

            /**
             * Encodes the specified UpdateJudgeResultResponse message. Does not implicitly {@link oj.biz.UpdateJudgeResultResponse.verify|verify} messages.
             * @param message UpdateJudgeResultResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.UpdateJudgeResultResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UpdateJudgeResultResponse message, length delimited. Does not implicitly {@link oj.biz.UpdateJudgeResultResponse.verify|verify} messages.
             * @param message UpdateJudgeResultResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.UpdateJudgeResultResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an UpdateJudgeResultResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.UpdateJudgeResultResponse & oj.biz.UpdateJudgeResultResponse.$Shape} UpdateJudgeResultResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.UpdateJudgeResultResponse & oj.biz.UpdateJudgeResultResponse.$Shape;

            /**
             * Decodes an UpdateJudgeResultResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.UpdateJudgeResultResponse & oj.biz.UpdateJudgeResultResponse.$Shape} UpdateJudgeResultResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.UpdateJudgeResultResponse & oj.biz.UpdateJudgeResultResponse.$Shape;

            /**
             * Verifies an UpdateJudgeResultResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an UpdateJudgeResultResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UpdateJudgeResultResponse
             */
            static fromObject(object: { [k: string]: any }): oj.biz.UpdateJudgeResultResponse;

            /**
             * Creates a plain object from an UpdateJudgeResultResponse message. Also converts values to other types if specified.
             * @param message UpdateJudgeResultResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.UpdateJudgeResultResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UpdateJudgeResultResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UpdateJudgeResultResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UpdateJudgeResultResponse {

            /** Properties of an UpdateJudgeResultResponse. */
            interface $Properties {

                /** UpdateJudgeResultResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** UpdateJudgeResultResponse outcome */
                outcome?: (oj.biz.JudgeResultPersistenceOutcome|null);

                /** UpdateJudgeResultResponse retryAfterMs */
                retryAfterMs?: (number|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an UpdateJudgeResultResponse. */
            type $Shape = oj.biz.UpdateJudgeResultResponse.$Properties;
        }

        /** Represents a SubmissionService */
        class SubmissionService extends $protobuf.rpc.Service {

            /**
             * Constructs a new SubmissionService service.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             */
            constructor(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean);

            /**
             * Creates new SubmissionService service using the specified rpc implementation.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             * @returns RPC service. Useful where requests and/or responses are streamed.
             */
            static create(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean): SubmissionService;

            /** Calls CreateSubmission. */
            createSubmission: oj.biz.SubmissionService.CreateSubmission;

            /** Calls CreateCustomTest. */
            createCustomTest: oj.biz.SubmissionService.CreateCustomTest;

            /** Calls GetSubmission. */
            getSubmission: oj.biz.SubmissionService.GetSubmission;

            /** Calls GetCustomTest. */
            getCustomTest: oj.biz.SubmissionService.GetCustomTest;

            /** Calls ListSubmissions. */
            listSubmissions: oj.biz.SubmissionService.ListSubmissions;

            /** Calls UpdateJudgeResult. */
            updateJudgeResult: oj.biz.SubmissionService.UpdateJudgeResult;

            /** Calls LegacyUpdateJudgeResult. */
            legacyUpdateJudgeResult: oj.biz.SubmissionService.LegacyUpdateJudgeResult;
        }

        namespace SubmissionService {

            /**
             * Callback as used by {@link oj.biz.SubmissionService#createSubmission}.
             * @param error Error, if any
             * @param [response] CreateSubmissionResponse
             */
            type CreateSubmissionCallback = (error: (Error|null), response?: oj.biz.CreateSubmissionResponse) => void;

            /** Calls CreateSubmission. */
            type CreateSubmission = {
              (request: oj.biz.ICreateSubmissionRequest, callback: oj.biz.SubmissionService.CreateSubmissionCallback): void;
              (request: oj.biz.ICreateSubmissionRequest): Promise<oj.biz.CreateSubmissionResponse>;
              readonly name: "CreateSubmission";
              readonly path: "/oj.biz.SubmissionService/CreateSubmission";
              readonly requestType: "CreateSubmissionRequest";
              readonly responseType: "CreateSubmissionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SubmissionService#createCustomTest}.
             * @param error Error, if any
             * @param [response] CreateCustomTestResponse
             */
            type CreateCustomTestCallback = (error: (Error|null), response?: oj.biz.CreateCustomTestResponse) => void;

            /** Calls CreateCustomTest. */
            type CreateCustomTest = {
              (request: oj.biz.ICreateCustomTestRequest, callback: oj.biz.SubmissionService.CreateCustomTestCallback): void;
              (request: oj.biz.ICreateCustomTestRequest): Promise<oj.biz.CreateCustomTestResponse>;
              readonly name: "CreateCustomTest";
              readonly path: "/oj.biz.SubmissionService/CreateCustomTest";
              readonly requestType: "CreateCustomTestRequest";
              readonly responseType: "CreateCustomTestResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SubmissionService#getSubmission}.
             * @param error Error, if any
             * @param [response] GetSubmissionResponse
             */
            type GetSubmissionCallback = (error: (Error|null), response?: oj.biz.GetSubmissionResponse) => void;

            /** Calls GetSubmission. */
            type GetSubmission = {
              (request: oj.biz.IGetSubmissionRequest, callback: oj.biz.SubmissionService.GetSubmissionCallback): void;
              (request: oj.biz.IGetSubmissionRequest): Promise<oj.biz.GetSubmissionResponse>;
              readonly name: "GetSubmission";
              readonly path: "/oj.biz.SubmissionService/GetSubmission";
              readonly requestType: "GetSubmissionRequest";
              readonly responseType: "GetSubmissionResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SubmissionService#getCustomTest}.
             * @param error Error, if any
             * @param [response] GetCustomTestResponse
             */
            type GetCustomTestCallback = (error: (Error|null), response?: oj.biz.GetCustomTestResponse) => void;

            /** Calls GetCustomTest. */
            type GetCustomTest = {
              (request: oj.biz.IGetCustomTestRequest, callback: oj.biz.SubmissionService.GetCustomTestCallback): void;
              (request: oj.biz.IGetCustomTestRequest): Promise<oj.biz.GetCustomTestResponse>;
              readonly name: "GetCustomTest";
              readonly path: "/oj.biz.SubmissionService/GetCustomTest";
              readonly requestType: "GetCustomTestRequest";
              readonly responseType: "GetCustomTestResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SubmissionService#listSubmissions}.
             * @param error Error, if any
             * @param [response] ListSubmissionsResponse
             */
            type ListSubmissionsCallback = (error: (Error|null), response?: oj.biz.ListSubmissionsResponse) => void;

            /** Calls ListSubmissions. */
            type ListSubmissions = {
              (request: oj.biz.IListSubmissionsRequest, callback: oj.biz.SubmissionService.ListSubmissionsCallback): void;
              (request: oj.biz.IListSubmissionsRequest): Promise<oj.biz.ListSubmissionsResponse>;
              readonly name: "ListSubmissions";
              readonly path: "/oj.biz.SubmissionService/ListSubmissions";
              readonly requestType: "ListSubmissionsRequest";
              readonly responseType: "ListSubmissionsResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SubmissionService#updateJudgeResult}.
             * @param error Error, if any
             * @param [response] UpdateJudgeResultResponse
             */
            type UpdateJudgeResultCallback = (error: (Error|null), response?: oj.biz.UpdateJudgeResultResponse) => void;

            /** Calls UpdateJudgeResult. */
            type UpdateJudgeResult = {
              (request: oj.biz.IUpdateJudgeResultRequest, callback: oj.biz.SubmissionService.UpdateJudgeResultCallback): void;
              (request: oj.biz.IUpdateJudgeResultRequest): Promise<oj.biz.UpdateJudgeResultResponse>;
              readonly name: "UpdateJudgeResult";
              readonly path: "/oj.biz.SubmissionService/UpdateJudgeResult";
              readonly requestType: "UpdateJudgeResultRequest";
              readonly responseType: "UpdateJudgeResultResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };

            /**
             * Callback as used by {@link oj.biz.SubmissionService#legacyUpdateJudgeResult}.
             * @param error Error, if any
             * @param [response] NullRsp
             */
            type LegacyUpdateJudgeResultCallback = (error: (Error|null), response?: oj_judge.NullRsp) => void;

            /** Calls LegacyUpdateJudgeResult. */
            type LegacyUpdateJudgeResult = {
              (request: oj_judge.IJudgeFinishedRequest, callback: oj.biz.SubmissionService.LegacyUpdateJudgeResultCallback): void;
              (request: oj_judge.IJudgeFinishedRequest): Promise<oj_judge.NullRsp>;
              readonly name: "LegacyUpdateJudgeResult";
              readonly path: "/oj.biz.SubmissionService/LegacyUpdateJudgeResult";
              readonly requestType: "oj_judge.JudgeFinishedRequest";
              readonly responseType: "oj_judge.NullRsp";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };
        }

        /**
         * Properties of an InvalidateStaticCacheRequest.
         * @deprecated Use oj.biz.InvalidateStaticCacheRequest.$Properties instead.
         */
        interface IInvalidateStaticCacheRequest extends oj.biz.InvalidateStaticCacheRequest.$Properties {
        }

        /** Represents an InvalidateStaticCacheRequest. */
        class InvalidateStaticCacheRequest {

            /**
             * Constructs a new InvalidateStaticCacheRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.biz.InvalidateStaticCacheRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** InvalidateStaticCacheRequest path. */
            path: string;

            /**
             * Creates a new InvalidateStaticCacheRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns InvalidateStaticCacheRequest instance
             */
            static create(properties: oj.biz.InvalidateStaticCacheRequest.$Shape): oj.biz.InvalidateStaticCacheRequest & oj.biz.InvalidateStaticCacheRequest.$Shape;
            static create(properties?: oj.biz.InvalidateStaticCacheRequest.$Properties): oj.biz.InvalidateStaticCacheRequest;

            /**
             * Encodes the specified InvalidateStaticCacheRequest message. Does not implicitly {@link oj.biz.InvalidateStaticCacheRequest.verify|verify} messages.
             * @param message InvalidateStaticCacheRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.biz.InvalidateStaticCacheRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified InvalidateStaticCacheRequest message, length delimited. Does not implicitly {@link oj.biz.InvalidateStaticCacheRequest.verify|verify} messages.
             * @param message InvalidateStaticCacheRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.biz.InvalidateStaticCacheRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an InvalidateStaticCacheRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.biz.InvalidateStaticCacheRequest & oj.biz.InvalidateStaticCacheRequest.$Shape} InvalidateStaticCacheRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.biz.InvalidateStaticCacheRequest & oj.biz.InvalidateStaticCacheRequest.$Shape;

            /**
             * Decodes an InvalidateStaticCacheRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.biz.InvalidateStaticCacheRequest & oj.biz.InvalidateStaticCacheRequest.$Shape} InvalidateStaticCacheRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.biz.InvalidateStaticCacheRequest & oj.biz.InvalidateStaticCacheRequest.$Shape;

            /**
             * Verifies an InvalidateStaticCacheRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an InvalidateStaticCacheRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns InvalidateStaticCacheRequest
             */
            static fromObject(object: { [k: string]: any }): oj.biz.InvalidateStaticCacheRequest;

            /**
             * Creates a plain object from an InvalidateStaticCacheRequest message. Also converts values to other types if specified.
             * @param message InvalidateStaticCacheRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.biz.InvalidateStaticCacheRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this InvalidateStaticCacheRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for InvalidateStaticCacheRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace InvalidateStaticCacheRequest {

            /** Properties of an InvalidateStaticCacheRequest. */
            interface $Properties {

                /** InvalidateStaticCacheRequest path */
                path?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an InvalidateStaticCacheRequest. */
            type $Shape = oj.biz.InvalidateStaticCacheRequest.$Properties;
        }

        /** Represents a StaticFileService */
        class StaticFileService extends $protobuf.rpc.Service {

            /**
             * Constructs a new StaticFileService service.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             */
            constructor(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean);

            /**
             * Creates new StaticFileService service using the specified rpc implementation.
             * @param rpcImpl RPC implementation
             * @param [requestDelimited=false] Whether requests are length-delimited
             * @param [responseDelimited=false] Whether responses are length-delimited
             * @returns RPC service. Useful where requests and/or responses are streamed.
             */
            static create(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean): StaticFileService;

            /** Calls InvalidateCache. */
            invalidateCache: oj.biz.StaticFileService.InvalidateCache;
        }

        namespace StaticFileService {

            /**
             * Callback as used by {@link oj.biz.StaticFileService#invalidateCache}.
             * @param error Error, if any
             * @param [response] EmptyResponse
             */
            type InvalidateCacheCallback = (error: (Error|null), response?: oj.common.EmptyResponse) => void;

            /** Calls InvalidateCache. */
            type InvalidateCache = {
              (request: oj.biz.IInvalidateStaticCacheRequest, callback: oj.biz.StaticFileService.InvalidateCacheCallback): void;
              (request: oj.biz.IInvalidateStaticCacheRequest): Promise<oj.common.EmptyResponse>;
              readonly name: "InvalidateCache";
              readonly path: "/oj.biz.StaticFileService/InvalidateCache";
              readonly requestType: "InvalidateStaticCacheRequest";
              readonly responseType: "oj.common.EmptyResponse";
              readonly requestStream: undefined;
              readonly responseStream: undefined;
            };
        }
    }

    /** Namespace common. */
    namespace common {

        /** ErrorCode enum. */
        enum ErrorCode {

            /** ERROR_CODE_UNSPECIFIED value */
            ERROR_CODE_UNSPECIFIED = 0,

            /** ERROR_CODE_OK value */
            ERROR_CODE_OK = 1,

            /** ERROR_CODE_INVALID_ARGUMENT value */
            ERROR_CODE_INVALID_ARGUMENT = 2,

            /** ERROR_CODE_UNAUTHENTICATED value */
            ERROR_CODE_UNAUTHENTICATED = 3,

            /** ERROR_CODE_PERMISSION_DENIED value */
            ERROR_CODE_PERMISSION_DENIED = 4,

            /** ERROR_CODE_NOT_FOUND value */
            ERROR_CODE_NOT_FOUND = 5,

            /** ERROR_CODE_CONFLICT value */
            ERROR_CODE_CONFLICT = 6,

            /** ERROR_CODE_RATE_LIMITED value */
            ERROR_CODE_RATE_LIMITED = 7,

            /** ERROR_CODE_UNAVAILABLE value */
            ERROR_CODE_UNAVAILABLE = 8,

            /** ERROR_CODE_INTERNAL value */
            ERROR_CODE_INTERNAL = 9
        }

        /**
         * Properties of a StatusResponse.
         * @deprecated Use oj.common.StatusResponse.$Properties instead.
         */
        interface IStatusResponse extends oj.common.StatusResponse.$Properties {
        }

        /** Represents a StatusResponse. */
        class StatusResponse {

            /**
             * Constructs a new StatusResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.StatusResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** StatusResponse code. */
            code: oj.common.ErrorCode;

            /** StatusResponse message. */
            message: string;

            /** StatusResponse retryable. */
            retryable: boolean;

            /**
             * Creates a new StatusResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns StatusResponse instance
             */
            static create(properties: oj.common.StatusResponse.$Shape): oj.common.StatusResponse & oj.common.StatusResponse.$Shape;
            static create(properties?: oj.common.StatusResponse.$Properties): oj.common.StatusResponse;

            /**
             * Encodes the specified StatusResponse message. Does not implicitly {@link oj.common.StatusResponse.verify|verify} messages.
             * @param message StatusResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.StatusResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified StatusResponse message, length delimited. Does not implicitly {@link oj.common.StatusResponse.verify|verify} messages.
             * @param message StatusResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.StatusResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a StatusResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.StatusResponse & oj.common.StatusResponse.$Shape} StatusResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.StatusResponse & oj.common.StatusResponse.$Shape;

            /**
             * Decodes a StatusResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.StatusResponse & oj.common.StatusResponse.$Shape} StatusResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.StatusResponse & oj.common.StatusResponse.$Shape;

            /**
             * Verifies a StatusResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a StatusResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns StatusResponse
             */
            static fromObject(object: { [k: string]: any }): oj.common.StatusResponse;

            /**
             * Creates a plain object from a StatusResponse message. Also converts values to other types if specified.
             * @param message StatusResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.StatusResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this StatusResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for StatusResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace StatusResponse {

            /** Properties of a StatusResponse. */
            interface $Properties {

                /** StatusResponse code */
                code?: (oj.common.ErrorCode|null);

                /** StatusResponse message */
                message?: (string|null);

                /** StatusResponse retryable */
                retryable?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a StatusResponse. */
            type $Shape = oj.common.StatusResponse.$Properties;
        }

        /**
         * Properties of a PageRequest.
         * @deprecated Use oj.common.PageRequest.$Properties instead.
         */
        interface IPageRequest extends oj.common.PageRequest.$Properties {
        }

        /** Represents a PageRequest. */
        class PageRequest {

            /**
             * Constructs a new PageRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.PageRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** PageRequest page. */
            page: number;

            /** PageRequest pageSize. */
            pageSize: number;

            /**
             * Creates a new PageRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns PageRequest instance
             */
            static create(properties: oj.common.PageRequest.$Shape): oj.common.PageRequest & oj.common.PageRequest.$Shape;
            static create(properties?: oj.common.PageRequest.$Properties): oj.common.PageRequest;

            /**
             * Encodes the specified PageRequest message. Does not implicitly {@link oj.common.PageRequest.verify|verify} messages.
             * @param message PageRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.PageRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified PageRequest message, length delimited. Does not implicitly {@link oj.common.PageRequest.verify|verify} messages.
             * @param message PageRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.PageRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a PageRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.PageRequest & oj.common.PageRequest.$Shape} PageRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.PageRequest & oj.common.PageRequest.$Shape;

            /**
             * Decodes a PageRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.PageRequest & oj.common.PageRequest.$Shape} PageRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.PageRequest & oj.common.PageRequest.$Shape;

            /**
             * Verifies a PageRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a PageRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns PageRequest
             */
            static fromObject(object: { [k: string]: any }): oj.common.PageRequest;

            /**
             * Creates a plain object from a PageRequest message. Also converts values to other types if specified.
             * @param message PageRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.PageRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this PageRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for PageRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace PageRequest {

            /** Properties of a PageRequest. */
            interface $Properties {

                /** PageRequest page */
                page?: (number|null);

                /** PageRequest pageSize */
                pageSize?: (number|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a PageRequest. */
            type $Shape = oj.common.PageRequest.$Properties;
        }

        /**
         * Properties of a PageResponse.
         * @deprecated Use oj.common.PageResponse.$Properties instead.
         */
        interface IPageResponse extends oj.common.PageResponse.$Properties {
        }

        /** Represents a PageResponse. */
        class PageResponse {

            /**
             * Constructs a new PageResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.PageResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** PageResponse totalItems. */
            totalItems: (Long|string);

            /** PageResponse page. */
            page: number;

            /** PageResponse pageSize. */
            pageSize: number;

            /** PageResponse totalPages. */
            totalPages: number;

            /**
             * Creates a new PageResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns PageResponse instance
             */
            static create(properties: oj.common.PageResponse.$Shape): oj.common.PageResponse & oj.common.PageResponse.$Shape;
            static create(properties?: oj.common.PageResponse.$Properties): oj.common.PageResponse;

            /**
             * Encodes the specified PageResponse message. Does not implicitly {@link oj.common.PageResponse.verify|verify} messages.
             * @param message PageResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.PageResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified PageResponse message, length delimited. Does not implicitly {@link oj.common.PageResponse.verify|verify} messages.
             * @param message PageResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.PageResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a PageResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.PageResponse & oj.common.PageResponse.$Shape} PageResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.PageResponse & oj.common.PageResponse.$Shape;

            /**
             * Decodes a PageResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.PageResponse & oj.common.PageResponse.$Shape} PageResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.PageResponse & oj.common.PageResponse.$Shape;

            /**
             * Verifies a PageResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a PageResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns PageResponse
             */
            static fromObject(object: { [k: string]: any }): oj.common.PageResponse;

            /**
             * Creates a plain object from a PageResponse message. Also converts values to other types if specified.
             * @param message PageResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.PageResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this PageResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for PageResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace PageResponse {

            /** Properties of a PageResponse. */
            interface $Properties {

                /** PageResponse totalItems */
                totalItems?: ((Long|string)|null);

                /** PageResponse page */
                page?: (number|null);

                /** PageResponse pageSize */
                pageSize?: (number|null);

                /** PageResponse totalPages */
                totalPages?: (number|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a PageResponse. */
            type $Shape = oj.common.PageResponse.$Properties;
        }

        /** SubmissionStatus enum. */
        enum SubmissionStatus {

            /** SUBMISSION_STATUS_UNSPECIFIED value */
            SUBMISSION_STATUS_UNSPECIFIED = 0,

            /** SUBMISSION_STATUS_PENDING value */
            SUBMISSION_STATUS_PENDING = 1,

            /** SUBMISSION_STATUS_QUEUED value */
            SUBMISSION_STATUS_QUEUED = 2,

            /** SUBMISSION_STATUS_JUDGING value */
            SUBMISSION_STATUS_JUDGING = 3,

            /** SUBMISSION_STATUS_ACCEPTED value */
            SUBMISSION_STATUS_ACCEPTED = 4,

            /** SUBMISSION_STATUS_WRONG_ANSWER value */
            SUBMISSION_STATUS_WRONG_ANSWER = 5,

            /** SUBMISSION_STATUS_COMPILE_ERROR value */
            SUBMISSION_STATUS_COMPILE_ERROR = 6,

            /** SUBMISSION_STATUS_MEMORY_LIMIT_EXCEEDED value */
            SUBMISSION_STATUS_MEMORY_LIMIT_EXCEEDED = 7,

            /** SUBMISSION_STATUS_TIME_LIMIT_EXCEEDED value */
            SUBMISSION_STATUS_TIME_LIMIT_EXCEEDED = 8,

            /** SUBMISSION_STATUS_RUNTIME_ERROR value */
            SUBMISSION_STATUS_RUNTIME_ERROR = 9,

            /** SUBMISSION_STATUS_SYSTEM_ERROR value */
            SUBMISSION_STATUS_SYSTEM_ERROR = 10,

            /** SUBMISSION_STATUS_CANCELLED value */
            SUBMISSION_STATUS_CANCELLED = 11
        }

        /**
         * Properties of a TestCaseResult.
         * @deprecated Use oj.common.TestCaseResult.$Properties instead.
         */
        interface ITestCaseResult extends oj.common.TestCaseResult.$Properties {
        }

        /** Represents a TestCaseResult. */
        class TestCaseResult {

            /**
             * Constructs a new TestCaseResult.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.TestCaseResult.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** TestCaseResult index. */
            index: number;

            /** TestCaseResult status. */
            status: oj.common.SubmissionStatus;

            /** TestCaseResult timeUsedMs. */
            timeUsedMs: (Long|string);

            /** TestCaseResult memoryUsedBytes. */
            memoryUsedBytes: (Long|string);

            /** TestCaseResult input. */
            input: string;

            /** TestCaseResult expectedOutput. */
            expectedOutput: string;

            /** TestCaseResult actualOutput. */
            actualOutput: string;

            /** TestCaseResult message. */
            message: string;

            /**
             * Creates a new TestCaseResult instance using the specified properties.
             * @param [properties] Properties to set
             * @returns TestCaseResult instance
             */
            static create(properties: oj.common.TestCaseResult.$Shape): oj.common.TestCaseResult & oj.common.TestCaseResult.$Shape;
            static create(properties?: oj.common.TestCaseResult.$Properties): oj.common.TestCaseResult;

            /**
             * Encodes the specified TestCaseResult message. Does not implicitly {@link oj.common.TestCaseResult.verify|verify} messages.
             * @param message TestCaseResult message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.TestCaseResult.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified TestCaseResult message, length delimited. Does not implicitly {@link oj.common.TestCaseResult.verify|verify} messages.
             * @param message TestCaseResult message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.TestCaseResult.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a TestCaseResult message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.TestCaseResult & oj.common.TestCaseResult.$Shape} TestCaseResult
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.TestCaseResult & oj.common.TestCaseResult.$Shape;

            /**
             * Decodes a TestCaseResult message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.TestCaseResult & oj.common.TestCaseResult.$Shape} TestCaseResult
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.TestCaseResult & oj.common.TestCaseResult.$Shape;

            /**
             * Verifies a TestCaseResult message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a TestCaseResult message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns TestCaseResult
             */
            static fromObject(object: { [k: string]: any }): oj.common.TestCaseResult;

            /**
             * Creates a plain object from a TestCaseResult message. Also converts values to other types if specified.
             * @param message TestCaseResult
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.TestCaseResult, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this TestCaseResult to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for TestCaseResult
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace TestCaseResult {

            /** Properties of a TestCaseResult. */
            interface $Properties {

                /** TestCaseResult index */
                index?: (number|null);

                /** TestCaseResult status */
                status?: (oj.common.SubmissionStatus|null);

                /** TestCaseResult timeUsedMs */
                timeUsedMs?: ((Long|string)|null);

                /** TestCaseResult memoryUsedBytes */
                memoryUsedBytes?: ((Long|string)|null);

                /** TestCaseResult input */
                input?: (string|null);

                /** TestCaseResult expectedOutput */
                expectedOutput?: (string|null);

                /** TestCaseResult actualOutput */
                actualOutput?: (string|null);

                /** TestCaseResult message */
                message?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a TestCaseResult. */
            type $Shape = oj.common.TestCaseResult.$Properties;
        }

        /**
         * Properties of a JudgeResult.
         * @deprecated Use oj.common.JudgeResult.$Properties instead.
         */
        interface IJudgeResult extends oj.common.JudgeResult.$Properties {
        }

        /** Represents a JudgeResult. */
        class JudgeResult {

            /**
             * Constructs a new JudgeResult.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.JudgeResult.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** JudgeResult status. */
            status: oj.common.SubmissionStatus;

            /** JudgeResult timeUsedMs. */
            timeUsedMs: (Long|string);

            /** JudgeResult memoryUsedBytes. */
            memoryUsedBytes: (Long|string);

            /** JudgeResult compileError. */
            compileError: string;

            /** JudgeResult testCaseResults. */
            testCaseResults: oj.common.TestCaseResult.$Properties[];

            /** JudgeResult completedAt. */
            completedAt: (Long|string);

            /**
             * Creates a new JudgeResult instance using the specified properties.
             * @param [properties] Properties to set
             * @returns JudgeResult instance
             */
            static create(properties: oj.common.JudgeResult.$Shape): oj.common.JudgeResult & oj.common.JudgeResult.$Shape;
            static create(properties?: oj.common.JudgeResult.$Properties): oj.common.JudgeResult;

            /**
             * Encodes the specified JudgeResult message. Does not implicitly {@link oj.common.JudgeResult.verify|verify} messages.
             * @param message JudgeResult message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.JudgeResult.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified JudgeResult message, length delimited. Does not implicitly {@link oj.common.JudgeResult.verify|verify} messages.
             * @param message JudgeResult message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.JudgeResult.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a JudgeResult message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.JudgeResult & oj.common.JudgeResult.$Shape} JudgeResult
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.JudgeResult & oj.common.JudgeResult.$Shape;

            /**
             * Decodes a JudgeResult message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.JudgeResult & oj.common.JudgeResult.$Shape} JudgeResult
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.JudgeResult & oj.common.JudgeResult.$Shape;

            /**
             * Verifies a JudgeResult message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a JudgeResult message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns JudgeResult
             */
            static fromObject(object: { [k: string]: any }): oj.common.JudgeResult;

            /**
             * Creates a plain object from a JudgeResult message. Also converts values to other types if specified.
             * @param message JudgeResult
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.JudgeResult, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this JudgeResult to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for JudgeResult
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace JudgeResult {

            /** Properties of a JudgeResult. */
            interface $Properties {

                /** JudgeResult status */
                status?: (oj.common.SubmissionStatus|null);

                /** JudgeResult timeUsedMs */
                timeUsedMs?: ((Long|string)|null);

                /** JudgeResult memoryUsedBytes */
                memoryUsedBytes?: ((Long|string)|null);

                /** JudgeResult compileError */
                compileError?: (string|null);

                /** JudgeResult testCaseResults */
                testCaseResults?: (oj.common.TestCaseResult.$Properties[]|null);

                /** JudgeResult completedAt */
                completedAt?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a JudgeResult. */
            type $Shape = oj.common.JudgeResult.$Properties;
        }

        /**
         * Properties of a User.
         * @deprecated Use oj.common.User.$Properties instead.
         */
        interface IUser extends oj.common.User.$Properties {
        }

        /** Represents a User. */
        class User {

            /**
             * Constructs a new User.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.User.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** User uid. */
            uid: number;

            /** User name. */
            name: string;

            /** User email. */
            email: string;

            /** User createTime. */
            createTime: (Long|string);

            /** User lastLogin. */
            lastLogin: (Long|string);

            /** User role. */
            role: number;

            /** User avatar. */
            avatar?: (oj.common.AvatarMetadata.$Properties|null);

            /** User disabled. */
            disabled: boolean;

            /**
             * Creates a new User instance using the specified properties.
             * @param [properties] Properties to set
             * @returns User instance
             */
            static create(properties: oj.common.User.$Shape): oj.common.User & oj.common.User.$Shape;
            static create(properties?: oj.common.User.$Properties): oj.common.User;

            /**
             * Encodes the specified User message. Does not implicitly {@link oj.common.User.verify|verify} messages.
             * @param message User message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.User.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified User message, length delimited. Does not implicitly {@link oj.common.User.verify|verify} messages.
             * @param message User message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.User.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a User message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.User & oj.common.User.$Shape} User
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.User & oj.common.User.$Shape;

            /**
             * Decodes a User message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.User & oj.common.User.$Shape} User
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.User & oj.common.User.$Shape;

            /**
             * Verifies a User message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a User message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns User
             */
            static fromObject(object: { [k: string]: any }): oj.common.User;

            /**
             * Creates a plain object from a User message. Also converts values to other types if specified.
             * @param message User
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.User, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this User to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for User
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace User {

            /** Properties of a User. */
            interface $Properties {

                /** User uid */
                uid?: (number|null);

                /** User name */
                name?: (string|null);

                /** User email */
                email?: (string|null);

                /** User createTime */
                createTime?: ((Long|string)|null);

                /** User lastLogin */
                lastLogin?: ((Long|string)|null);

                /** User role */
                role?: (number|null);

                /** User avatar */
                avatar?: (oj.common.AvatarMetadata.$Properties|null);

                /** User disabled */
                disabled?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a User. */
            type $Shape = oj.common.User.$Properties;
        }

        /**
         * Properties of an AvatarMetadata.
         * @deprecated Use oj.common.AvatarMetadata.$Properties instead.
         */
        interface IAvatarMetadata extends oj.common.AvatarMetadata.$Properties {
        }

        /** Represents an AvatarMetadata. */
        class AvatarMetadata {

            /**
             * Constructs a new AvatarMetadata.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.AvatarMetadata.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AvatarMetadata url. */
            url: string;

            /** AvatarMetadata contentType. */
            contentType: string;

            /** AvatarMetadata sizeBytes. */
            sizeBytes: (Long|string);

            /** AvatarMetadata updatedAt. */
            updatedAt: (Long|string);

            /**
             * Creates a new AvatarMetadata instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AvatarMetadata instance
             */
            static create(properties: oj.common.AvatarMetadata.$Shape): oj.common.AvatarMetadata & oj.common.AvatarMetadata.$Shape;
            static create(properties?: oj.common.AvatarMetadata.$Properties): oj.common.AvatarMetadata;

            /**
             * Encodes the specified AvatarMetadata message. Does not implicitly {@link oj.common.AvatarMetadata.verify|verify} messages.
             * @param message AvatarMetadata message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.AvatarMetadata.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AvatarMetadata message, length delimited. Does not implicitly {@link oj.common.AvatarMetadata.verify|verify} messages.
             * @param message AvatarMetadata message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.AvatarMetadata.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AvatarMetadata message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.AvatarMetadata & oj.common.AvatarMetadata.$Shape} AvatarMetadata
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.AvatarMetadata & oj.common.AvatarMetadata.$Shape;

            /**
             * Decodes an AvatarMetadata message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.AvatarMetadata & oj.common.AvatarMetadata.$Shape} AvatarMetadata
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.AvatarMetadata & oj.common.AvatarMetadata.$Shape;

            /**
             * Verifies an AvatarMetadata message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AvatarMetadata message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AvatarMetadata
             */
            static fromObject(object: { [k: string]: any }): oj.common.AvatarMetadata;

            /**
             * Creates a plain object from an AvatarMetadata message. Also converts values to other types if specified.
             * @param message AvatarMetadata
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.AvatarMetadata, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AvatarMetadata to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AvatarMetadata
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AvatarMetadata {

            /** Properties of an AvatarMetadata. */
            interface $Properties {

                /** AvatarMetadata url */
                url?: (string|null);

                /** AvatarMetadata contentType */
                contentType?: (string|null);

                /** AvatarMetadata sizeBytes */
                sizeBytes?: ((Long|string)|null);

                /** AvatarMetadata updatedAt */
                updatedAt?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AvatarMetadata. */
            type $Shape = oj.common.AvatarMetadata.$Properties;
        }

        /**
         * Properties of a UserStatistics.
         * @deprecated Use oj.common.UserStatistics.$Properties instead.
         */
        interface IUserStatistics extends oj.common.UserStatistics.$Properties {
        }

        /** Represents a UserStatistics. */
        class UserStatistics {

            /**
             * Constructs a new UserStatistics.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.UserStatistics.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** UserStatistics totalSubmissions. */
            totalSubmissions: (Long|string);

            /** UserStatistics acceptedSubmissions. */
            acceptedSubmissions: (Long|string);

            /** UserStatistics acceptedQuestions. */
            acceptedQuestions: (Long|string);

            /** UserStatistics totalSolutions. */
            totalSolutions: (Long|string);

            /** UserStatistics totalLikesReceived. */
            totalLikesReceived: (Long|string);

            /**
             * Creates a new UserStatistics instance using the specified properties.
             * @param [properties] Properties to set
             * @returns UserStatistics instance
             */
            static create(properties: oj.common.UserStatistics.$Shape): oj.common.UserStatistics & oj.common.UserStatistics.$Shape;
            static create(properties?: oj.common.UserStatistics.$Properties): oj.common.UserStatistics;

            /**
             * Encodes the specified UserStatistics message. Does not implicitly {@link oj.common.UserStatistics.verify|verify} messages.
             * @param message UserStatistics message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.UserStatistics.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified UserStatistics message, length delimited. Does not implicitly {@link oj.common.UserStatistics.verify|verify} messages.
             * @param message UserStatistics message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.UserStatistics.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a UserStatistics message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.UserStatistics & oj.common.UserStatistics.$Shape} UserStatistics
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.UserStatistics & oj.common.UserStatistics.$Shape;

            /**
             * Decodes a UserStatistics message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.UserStatistics & oj.common.UserStatistics.$Shape} UserStatistics
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.UserStatistics & oj.common.UserStatistics.$Shape;

            /**
             * Verifies a UserStatistics message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a UserStatistics message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns UserStatistics
             */
            static fromObject(object: { [k: string]: any }): oj.common.UserStatistics;

            /**
             * Creates a plain object from a UserStatistics message. Also converts values to other types if specified.
             * @param message UserStatistics
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.UserStatistics, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this UserStatistics to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for UserStatistics
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace UserStatistics {

            /** Properties of a UserStatistics. */
            interface $Properties {

                /** UserStatistics totalSubmissions */
                totalSubmissions?: ((Long|string)|null);

                /** UserStatistics acceptedSubmissions */
                acceptedSubmissions?: ((Long|string)|null);

                /** UserStatistics acceptedQuestions */
                acceptedQuestions?: ((Long|string)|null);

                /** UserStatistics totalSolutions */
                totalSolutions?: ((Long|string)|null);

                /** UserStatistics totalLikesReceived */
                totalLikesReceived?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a UserStatistics. */
            type $Shape = oj.common.UserStatistics.$Properties;
        }

        /**
         * Properties of a Question.
         * @deprecated Use oj.common.Question.$Properties instead.
         */
        interface IQuestion extends oj.common.Question.$Properties {
        }

        /** Represents a Question. */
        class Question {

            /**
             * Constructs a new Question.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.Question.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** Question id. */
            id: string;

            /** Question title. */
            title: string;

            /** Question description. */
            description: string;

            /** Question difficulty. */
            difficulty: string;

            /** Question timeLimitMs. */
            timeLimitMs: number;

            /** Question memoryLimitMb. */
            memoryLimitMb: number;

            /** Question createTime. */
            createTime: (Long|string);

            /** Question updateTime. */
            updateTime: (Long|string);

            /** Question visible. */
            visible: boolean;

            /**
             * Creates a new Question instance using the specified properties.
             * @param [properties] Properties to set
             * @returns Question instance
             */
            static create(properties: oj.common.Question.$Shape): oj.common.Question & oj.common.Question.$Shape;
            static create(properties?: oj.common.Question.$Properties): oj.common.Question;

            /**
             * Encodes the specified Question message. Does not implicitly {@link oj.common.Question.verify|verify} messages.
             * @param message Question message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.Question.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified Question message, length delimited. Does not implicitly {@link oj.common.Question.verify|verify} messages.
             * @param message Question message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.Question.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a Question message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.Question & oj.common.Question.$Shape} Question
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.Question & oj.common.Question.$Shape;

            /**
             * Decodes a Question message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.Question & oj.common.Question.$Shape} Question
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.Question & oj.common.Question.$Shape;

            /**
             * Verifies a Question message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a Question message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns Question
             */
            static fromObject(object: { [k: string]: any }): oj.common.Question;

            /**
             * Creates a plain object from a Question message. Also converts values to other types if specified.
             * @param message Question
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.Question, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this Question to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for Question
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace Question {

            /** Properties of a Question. */
            interface $Properties {

                /** Question id */
                id?: (string|null);

                /** Question title */
                title?: (string|null);

                /** Question description */
                description?: (string|null);

                /** Question difficulty */
                difficulty?: (string|null);

                /** Question timeLimitMs */
                timeLimitMs?: (number|null);

                /** Question memoryLimitMb */
                memoryLimitMb?: (number|null);

                /** Question createTime */
                createTime?: ((Long|string)|null);

                /** Question updateTime */
                updateTime?: ((Long|string)|null);

                /** Question visible */
                visible?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a Question. */
            type $Shape = oj.common.Question.$Properties;
        }

        /**
         * Properties of a TestCase.
         * @deprecated Use oj.common.TestCase.$Properties instead.
         */
        interface ITestCase extends oj.common.TestCase.$Properties {
        }

        /** Represents a TestCase. */
        class TestCase {

            /**
             * Constructs a new TestCase.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.TestCase.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** TestCase testCaseId. */
            testCaseId: (Long|string);

            /** TestCase questionId. */
            questionId: string;

            /** TestCase ordinal. */
            ordinal: number;

            /** TestCase input. */
            input: string;

            /** TestCase expectedOutput. */
            expectedOutput: string;

            /** TestCase isSample. */
            isSample: boolean;

            /**
             * Creates a new TestCase instance using the specified properties.
             * @param [properties] Properties to set
             * @returns TestCase instance
             */
            static create(properties: oj.common.TestCase.$Shape): oj.common.TestCase & oj.common.TestCase.$Shape;
            static create(properties?: oj.common.TestCase.$Properties): oj.common.TestCase;

            /**
             * Encodes the specified TestCase message. Does not implicitly {@link oj.common.TestCase.verify|verify} messages.
             * @param message TestCase message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.TestCase.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified TestCase message, length delimited. Does not implicitly {@link oj.common.TestCase.verify|verify} messages.
             * @param message TestCase message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.TestCase.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a TestCase message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.TestCase & oj.common.TestCase.$Shape} TestCase
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.TestCase & oj.common.TestCase.$Shape;

            /**
             * Decodes a TestCase message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.TestCase & oj.common.TestCase.$Shape} TestCase
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.TestCase & oj.common.TestCase.$Shape;

            /**
             * Verifies a TestCase message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a TestCase message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns TestCase
             */
            static fromObject(object: { [k: string]: any }): oj.common.TestCase;

            /**
             * Creates a plain object from a TestCase message. Also converts values to other types if specified.
             * @param message TestCase
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.TestCase, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this TestCase to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for TestCase
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace TestCase {

            /** Properties of a TestCase. */
            interface $Properties {

                /** TestCase testCaseId */
                testCaseId?: ((Long|string)|null);

                /** TestCase questionId */
                questionId?: (string|null);

                /** TestCase ordinal */
                ordinal?: (number|null);

                /** TestCase input */
                input?: (string|null);

                /** TestCase expectedOutput */
                expectedOutput?: (string|null);

                /** TestCase isSample */
                isSample?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a TestCase. */
            type $Shape = oj.common.TestCase.$Properties;
        }

        /**
         * Properties of a Solution.
         * @deprecated Use oj.common.Solution.$Properties instead.
         */
        interface ISolution extends oj.common.Solution.$Properties {
        }

        /** Represents a Solution. */
        class Solution {

            /**
             * Constructs a new Solution.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.Solution.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** Solution id. */
            id: (Long|string);

            /** Solution questionId. */
            questionId: string;

            /** Solution userId. */
            userId: number;

            /** Solution title. */
            title: string;

            /** Solution contentMd. */
            contentMd: string;

            /** Solution likeCount. */
            likeCount: (Long|string);

            /** Solution favoriteCount. */
            favoriteCount: (Long|string);

            /** Solution commentCount. */
            commentCount: (Long|string);

            /** Solution moderationStatus. */
            moderationStatus: string;

            /** Solution createdAt. */
            createdAt: (Long|string);

            /** Solution updatedAt. */
            updatedAt: (Long|string);

            /**
             * Creates a new Solution instance using the specified properties.
             * @param [properties] Properties to set
             * @returns Solution instance
             */
            static create(properties: oj.common.Solution.$Shape): oj.common.Solution & oj.common.Solution.$Shape;
            static create(properties?: oj.common.Solution.$Properties): oj.common.Solution;

            /**
             * Encodes the specified Solution message. Does not implicitly {@link oj.common.Solution.verify|verify} messages.
             * @param message Solution message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.Solution.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified Solution message, length delimited. Does not implicitly {@link oj.common.Solution.verify|verify} messages.
             * @param message Solution message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.Solution.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a Solution message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.Solution & oj.common.Solution.$Shape} Solution
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.Solution & oj.common.Solution.$Shape;

            /**
             * Decodes a Solution message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.Solution & oj.common.Solution.$Shape} Solution
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.Solution & oj.common.Solution.$Shape;

            /**
             * Verifies a Solution message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a Solution message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns Solution
             */
            static fromObject(object: { [k: string]: any }): oj.common.Solution;

            /**
             * Creates a plain object from a Solution message. Also converts values to other types if specified.
             * @param message Solution
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.Solution, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this Solution to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for Solution
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace Solution {

            /** Properties of a Solution. */
            interface $Properties {

                /** Solution id */
                id?: ((Long|string)|null);

                /** Solution questionId */
                questionId?: (string|null);

                /** Solution userId */
                userId?: (number|null);

                /** Solution title */
                title?: (string|null);

                /** Solution contentMd */
                contentMd?: (string|null);

                /** Solution likeCount */
                likeCount?: ((Long|string)|null);

                /** Solution favoriteCount */
                favoriteCount?: ((Long|string)|null);

                /** Solution commentCount */
                commentCount?: ((Long|string)|null);

                /** Solution moderationStatus */
                moderationStatus?: (string|null);

                /** Solution createdAt */
                createdAt?: ((Long|string)|null);

                /** Solution updatedAt */
                updatedAt?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a Solution. */
            type $Shape = oj.common.Solution.$Properties;
        }

        /**
         * Properties of a SolutionActionState.
         * @deprecated Use oj.common.SolutionActionState.$Properties instead.
         */
        interface ISolutionActionState extends oj.common.SolutionActionState.$Properties {
        }

        /** Represents a SolutionActionState. */
        class SolutionActionState {

            /**
             * Constructs a new SolutionActionState.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.SolutionActionState.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** SolutionActionState liked. */
            liked: boolean;

            /** SolutionActionState favorited. */
            favorited: boolean;

            /**
             * Creates a new SolutionActionState instance using the specified properties.
             * @param [properties] Properties to set
             * @returns SolutionActionState instance
             */
            static create(properties: oj.common.SolutionActionState.$Shape): oj.common.SolutionActionState & oj.common.SolutionActionState.$Shape;
            static create(properties?: oj.common.SolutionActionState.$Properties): oj.common.SolutionActionState;

            /**
             * Encodes the specified SolutionActionState message. Does not implicitly {@link oj.common.SolutionActionState.verify|verify} messages.
             * @param message SolutionActionState message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.SolutionActionState.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified SolutionActionState message, length delimited. Does not implicitly {@link oj.common.SolutionActionState.verify|verify} messages.
             * @param message SolutionActionState message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.SolutionActionState.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a SolutionActionState message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.SolutionActionState & oj.common.SolutionActionState.$Shape} SolutionActionState
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.SolutionActionState & oj.common.SolutionActionState.$Shape;

            /**
             * Decodes a SolutionActionState message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.SolutionActionState & oj.common.SolutionActionState.$Shape} SolutionActionState
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.SolutionActionState & oj.common.SolutionActionState.$Shape;

            /**
             * Verifies a SolutionActionState message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a SolutionActionState message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns SolutionActionState
             */
            static fromObject(object: { [k: string]: any }): oj.common.SolutionActionState;

            /**
             * Creates a plain object from a SolutionActionState message. Also converts values to other types if specified.
             * @param message SolutionActionState
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.SolutionActionState, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this SolutionActionState to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for SolutionActionState
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace SolutionActionState {

            /** Properties of a SolutionActionState. */
            interface $Properties {

                /** SolutionActionState liked */
                liked?: (boolean|null);

                /** SolutionActionState favorited */
                favorited?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a SolutionActionState. */
            type $Shape = oj.common.SolutionActionState.$Properties;
        }

        /**
         * Properties of a Comment.
         * @deprecated Use oj.common.Comment.$Properties instead.
         */
        interface IComment extends oj.common.Comment.$Properties {
        }

        /** Represents a Comment. */
        class Comment {

            /**
             * Constructs a new Comment.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.Comment.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** Comment id. */
            id: (Long|string);

            /** Comment solutionId. */
            solutionId: (Long|string);

            /** Comment userId. */
            userId: number;

            /** Comment parentId. */
            parentId: (Long|string);

            /** Comment replyToUserId. */
            replyToUserId: number;

            /** Comment content. */
            content: string;

            /** Comment likeCount. */
            likeCount: (Long|string);

            /** Comment favoriteCount. */
            favoriteCount: (Long|string);

            /** Comment isEdited. */
            isEdited: boolean;

            /** Comment createdAt. */
            createdAt: (Long|string);

            /** Comment updatedAt. */
            updatedAt: (Long|string);

            /**
             * Creates a new Comment instance using the specified properties.
             * @param [properties] Properties to set
             * @returns Comment instance
             */
            static create(properties: oj.common.Comment.$Shape): oj.common.Comment & oj.common.Comment.$Shape;
            static create(properties?: oj.common.Comment.$Properties): oj.common.Comment;

            /**
             * Encodes the specified Comment message. Does not implicitly {@link oj.common.Comment.verify|verify} messages.
             * @param message Comment message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.Comment.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified Comment message, length delimited. Does not implicitly {@link oj.common.Comment.verify|verify} messages.
             * @param message Comment message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.Comment.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a Comment message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.Comment & oj.common.Comment.$Shape} Comment
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.Comment & oj.common.Comment.$Shape;

            /**
             * Decodes a Comment message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.Comment & oj.common.Comment.$Shape} Comment
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.Comment & oj.common.Comment.$Shape;

            /**
             * Verifies a Comment message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a Comment message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns Comment
             */
            static fromObject(object: { [k: string]: any }): oj.common.Comment;

            /**
             * Creates a plain object from a Comment message. Also converts values to other types if specified.
             * @param message Comment
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.Comment, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this Comment to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for Comment
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace Comment {

            /** Properties of a Comment. */
            interface $Properties {

                /** Comment id */
                id?: ((Long|string)|null);

                /** Comment solutionId */
                solutionId?: ((Long|string)|null);

                /** Comment userId */
                userId?: (number|null);

                /** Comment parentId */
                parentId?: ((Long|string)|null);

                /** Comment replyToUserId */
                replyToUserId?: (number|null);

                /** Comment content */
                content?: (string|null);

                /** Comment likeCount */
                likeCount?: ((Long|string)|null);

                /** Comment favoriteCount */
                favoriteCount?: ((Long|string)|null);

                /** Comment isEdited */
                isEdited?: (boolean|null);

                /** Comment createdAt */
                createdAt?: ((Long|string)|null);

                /** Comment updatedAt */
                updatedAt?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a Comment. */
            type $Shape = oj.common.Comment.$Properties;
        }

        /**
         * Properties of a CommentActionState.
         * @deprecated Use oj.common.CommentActionState.$Properties instead.
         */
        interface ICommentActionState extends oj.common.CommentActionState.$Properties {
        }

        /** Represents a CommentActionState. */
        class CommentActionState {

            /**
             * Constructs a new CommentActionState.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.CommentActionState.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CommentActionState commentId. */
            commentId: (Long|string);

            /** CommentActionState liked. */
            liked: boolean;

            /** CommentActionState favorited. */
            favorited: boolean;

            /**
             * Creates a new CommentActionState instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CommentActionState instance
             */
            static create(properties: oj.common.CommentActionState.$Shape): oj.common.CommentActionState & oj.common.CommentActionState.$Shape;
            static create(properties?: oj.common.CommentActionState.$Properties): oj.common.CommentActionState;

            /**
             * Encodes the specified CommentActionState message. Does not implicitly {@link oj.common.CommentActionState.verify|verify} messages.
             * @param message CommentActionState message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.CommentActionState.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CommentActionState message, length delimited. Does not implicitly {@link oj.common.CommentActionState.verify|verify} messages.
             * @param message CommentActionState message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.CommentActionState.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CommentActionState message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.CommentActionState & oj.common.CommentActionState.$Shape} CommentActionState
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.CommentActionState & oj.common.CommentActionState.$Shape;

            /**
             * Decodes a CommentActionState message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.CommentActionState & oj.common.CommentActionState.$Shape} CommentActionState
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.CommentActionState & oj.common.CommentActionState.$Shape;

            /**
             * Verifies a CommentActionState message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CommentActionState message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CommentActionState
             */
            static fromObject(object: { [k: string]: any }): oj.common.CommentActionState;

            /**
             * Creates a plain object from a CommentActionState message. Also converts values to other types if specified.
             * @param message CommentActionState
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.CommentActionState, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CommentActionState to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CommentActionState
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CommentActionState {

            /** Properties of a CommentActionState. */
            interface $Properties {

                /** CommentActionState commentId */
                commentId?: ((Long|string)|null);

                /** CommentActionState liked */
                liked?: (boolean|null);

                /** CommentActionState favorited */
                favorited?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CommentActionState. */
            type $Shape = oj.common.CommentActionState.$Properties;
        }

        /**
         * Properties of a Submission.
         * @deprecated Use oj.common.Submission.$Properties instead.
         */
        interface ISubmission extends oj.common.Submission.$Properties {
        }

        /** Represents a Submission. */
        class Submission {

            /**
             * Constructs a new Submission.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.Submission.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** Submission submissionId. */
            submissionId: (Long|string);

            /** Submission userId. */
            userId: number;

            /** Submission questionId. */
            questionId: string;

            /** Submission code. */
            code: string;

            /** Submission language. */
            language: string;

            /** Submission status. */
            status: oj.common.SubmissionStatus;

            /** Submission result. */
            result?: (oj.common.JudgeResult.$Properties|null);

            /** Submission createdAt. */
            createdAt: (Long|string);

            /** Submission completedAt. */
            completedAt: (Long|string);

            /** Submission resultVersion. */
            resultVersion: (Long|string);

            /**
             * Creates a new Submission instance using the specified properties.
             * @param [properties] Properties to set
             * @returns Submission instance
             */
            static create(properties: oj.common.Submission.$Shape): oj.common.Submission & oj.common.Submission.$Shape;
            static create(properties?: oj.common.Submission.$Properties): oj.common.Submission;

            /**
             * Encodes the specified Submission message. Does not implicitly {@link oj.common.Submission.verify|verify} messages.
             * @param message Submission message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.Submission.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified Submission message, length delimited. Does not implicitly {@link oj.common.Submission.verify|verify} messages.
             * @param message Submission message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.Submission.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a Submission message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.Submission & oj.common.Submission.$Shape} Submission
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.Submission & oj.common.Submission.$Shape;

            /**
             * Decodes a Submission message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.Submission & oj.common.Submission.$Shape} Submission
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.Submission & oj.common.Submission.$Shape;

            /**
             * Verifies a Submission message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a Submission message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns Submission
             */
            static fromObject(object: { [k: string]: any }): oj.common.Submission;

            /**
             * Creates a plain object from a Submission message. Also converts values to other types if specified.
             * @param message Submission
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.Submission, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this Submission to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for Submission
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace Submission {

            /** Properties of a Submission. */
            interface $Properties {

                /** Submission submissionId */
                submissionId?: ((Long|string)|null);

                /** Submission userId */
                userId?: (number|null);

                /** Submission questionId */
                questionId?: (string|null);

                /** Submission code */
                code?: (string|null);

                /** Submission language */
                language?: (string|null);

                /** Submission status */
                status?: (oj.common.SubmissionStatus|null);

                /** Submission result */
                result?: (oj.common.JudgeResult.$Properties|null);

                /** Submission createdAt */
                createdAt?: ((Long|string)|null);

                /** Submission completedAt */
                completedAt?: ((Long|string)|null);

                /** Submission resultVersion */
                resultVersion?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a Submission. */
            type $Shape = oj.common.Submission.$Properties;
        }

        /**
         * Properties of an AdminAccount.
         * @deprecated Use oj.common.AdminAccount.$Properties instead.
         */
        interface IAdminAccount extends oj.common.AdminAccount.$Properties {
        }

        /** Represents an AdminAccount. */
        class AdminAccount {

            /**
             * Constructs a new AdminAccount.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.AdminAccount.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminAccount adminId. */
            adminId: number;

            /** AdminAccount uid. */
            uid: number;

            /** AdminAccount role. */
            role: string;

            /** AdminAccount createdAt. */
            createdAt: (Long|string);

            /** AdminAccount disabled. */
            disabled: boolean;

            /**
             * Creates a new AdminAccount instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminAccount instance
             */
            static create(properties: oj.common.AdminAccount.$Shape): oj.common.AdminAccount & oj.common.AdminAccount.$Shape;
            static create(properties?: oj.common.AdminAccount.$Properties): oj.common.AdminAccount;

            /**
             * Encodes the specified AdminAccount message. Does not implicitly {@link oj.common.AdminAccount.verify|verify} messages.
             * @param message AdminAccount message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.AdminAccount.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminAccount message, length delimited. Does not implicitly {@link oj.common.AdminAccount.verify|verify} messages.
             * @param message AdminAccount message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.AdminAccount.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminAccount message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.AdminAccount & oj.common.AdminAccount.$Shape} AdminAccount
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.AdminAccount & oj.common.AdminAccount.$Shape;

            /**
             * Decodes an AdminAccount message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.AdminAccount & oj.common.AdminAccount.$Shape} AdminAccount
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.AdminAccount & oj.common.AdminAccount.$Shape;

            /**
             * Verifies an AdminAccount message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminAccount message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminAccount
             */
            static fromObject(object: { [k: string]: any }): oj.common.AdminAccount;

            /**
             * Creates a plain object from an AdminAccount message. Also converts values to other types if specified.
             * @param message AdminAccount
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.AdminAccount, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminAccount to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminAccount
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminAccount {

            /** Properties of an AdminAccount. */
            interface $Properties {

                /** AdminAccount adminId */
                adminId?: (number|null);

                /** AdminAccount uid */
                uid?: (number|null);

                /** AdminAccount role */
                role?: (string|null);

                /** AdminAccount createdAt */
                createdAt?: ((Long|string)|null);

                /** AdminAccount disabled */
                disabled?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminAccount. */
            type $Shape = oj.common.AdminAccount.$Properties;
        }

        /**
         * Properties of an AdminAuditLog.
         * @deprecated Use oj.common.AdminAuditLog.$Properties instead.
         */
        interface IAdminAuditLog extends oj.common.AdminAuditLog.$Properties {
        }

        /** Represents an AdminAuditLog. */
        class AdminAuditLog {

            /**
             * Constructs a new AdminAuditLog.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.AdminAuditLog.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AdminAuditLog logId. */
            logId: (Long|string);

            /** AdminAuditLog requestId. */
            requestId: string;

            /** AdminAuditLog operatorAdminId. */
            operatorAdminId: number;

            /** AdminAuditLog operatorRole. */
            operatorRole: string;

            /** AdminAuditLog action. */
            action: string;

            /** AdminAuditLog resourceType. */
            resourceType: string;

            /** AdminAuditLog result. */
            result: string;

            /** AdminAuditLog createdAt. */
            createdAt: (Long|string);

            /** AdminAuditLog payloadText. */
            payloadText: string;

            /**
             * Creates a new AdminAuditLog instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AdminAuditLog instance
             */
            static create(properties: oj.common.AdminAuditLog.$Shape): oj.common.AdminAuditLog & oj.common.AdminAuditLog.$Shape;
            static create(properties?: oj.common.AdminAuditLog.$Properties): oj.common.AdminAuditLog;

            /**
             * Encodes the specified AdminAuditLog message. Does not implicitly {@link oj.common.AdminAuditLog.verify|verify} messages.
             * @param message AdminAuditLog message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.AdminAuditLog.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AdminAuditLog message, length delimited. Does not implicitly {@link oj.common.AdminAuditLog.verify|verify} messages.
             * @param message AdminAuditLog message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.AdminAuditLog.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AdminAuditLog message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.AdminAuditLog & oj.common.AdminAuditLog.$Shape} AdminAuditLog
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.AdminAuditLog & oj.common.AdminAuditLog.$Shape;

            /**
             * Decodes an AdminAuditLog message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.AdminAuditLog & oj.common.AdminAuditLog.$Shape} AdminAuditLog
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.AdminAuditLog & oj.common.AdminAuditLog.$Shape;

            /**
             * Verifies an AdminAuditLog message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AdminAuditLog message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AdminAuditLog
             */
            static fromObject(object: { [k: string]: any }): oj.common.AdminAuditLog;

            /**
             * Creates a plain object from an AdminAuditLog message. Also converts values to other types if specified.
             * @param message AdminAuditLog
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.AdminAuditLog, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AdminAuditLog to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AdminAuditLog
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AdminAuditLog {

            /** Properties of an AdminAuditLog. */
            interface $Properties {

                /** AdminAuditLog logId */
                logId?: ((Long|string)|null);

                /** AdminAuditLog requestId */
                requestId?: (string|null);

                /** AdminAuditLog operatorAdminId */
                operatorAdminId?: (number|null);

                /** AdminAuditLog operatorRole */
                operatorRole?: (string|null);

                /** AdminAuditLog action */
                action?: (string|null);

                /** AdminAuditLog resourceType */
                resourceType?: (string|null);

                /** AdminAuditLog result */
                result?: (string|null);

                /** AdminAuditLog createdAt */
                createdAt?: ((Long|string)|null);

                /** AdminAuditLog payloadText */
                payloadText?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AdminAuditLog. */
            type $Shape = oj.common.AdminAuditLog.$Properties;
        }

        /**
         * Properties of a CacheMetrics.
         * @deprecated Use oj.common.CacheMetrics.$Properties instead.
         */
        interface ICacheMetrics extends oj.common.CacheMetrics.$Properties {
        }

        /** Represents a CacheMetrics. */
        class CacheMetrics {

            /**
             * Constructs a new CacheMetrics.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.CacheMetrics.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** CacheMetrics hits. */
            hits: (Long|string);

            /** CacheMetrics misses. */
            misses: (Long|string);

            /** CacheMetrics errors. */
            errors: (Long|string);

            /** CacheMetrics invalidations. */
            invalidations: (Long|string);

            /** CacheMetrics sampledAt. */
            sampledAt: (Long|string);

            /**
             * Creates a new CacheMetrics instance using the specified properties.
             * @param [properties] Properties to set
             * @returns CacheMetrics instance
             */
            static create(properties: oj.common.CacheMetrics.$Shape): oj.common.CacheMetrics & oj.common.CacheMetrics.$Shape;
            static create(properties?: oj.common.CacheMetrics.$Properties): oj.common.CacheMetrics;

            /**
             * Encodes the specified CacheMetrics message. Does not implicitly {@link oj.common.CacheMetrics.verify|verify} messages.
             * @param message CacheMetrics message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.CacheMetrics.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified CacheMetrics message, length delimited. Does not implicitly {@link oj.common.CacheMetrics.verify|verify} messages.
             * @param message CacheMetrics message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.CacheMetrics.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a CacheMetrics message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.CacheMetrics & oj.common.CacheMetrics.$Shape} CacheMetrics
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.CacheMetrics & oj.common.CacheMetrics.$Shape;

            /**
             * Decodes a CacheMetrics message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.CacheMetrics & oj.common.CacheMetrics.$Shape} CacheMetrics
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.CacheMetrics & oj.common.CacheMetrics.$Shape;

            /**
             * Verifies a CacheMetrics message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a CacheMetrics message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns CacheMetrics
             */
            static fromObject(object: { [k: string]: any }): oj.common.CacheMetrics;

            /**
             * Creates a plain object from a CacheMetrics message. Also converts values to other types if specified.
             * @param message CacheMetrics
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.CacheMetrics, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this CacheMetrics to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for CacheMetrics
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace CacheMetrics {

            /** Properties of a CacheMetrics. */
            interface $Properties {

                /** CacheMetrics hits */
                hits?: ((Long|string)|null);

                /** CacheMetrics misses */
                misses?: ((Long|string)|null);

                /** CacheMetrics errors */
                errors?: ((Long|string)|null);

                /** CacheMetrics invalidations */
                invalidations?: ((Long|string)|null);

                /** CacheMetrics sampledAt */
                sampledAt?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a CacheMetrics. */
            type $Shape = oj.common.CacheMetrics.$Properties;
        }

        /**
         * Properties of an EmptyRequest.
         * @deprecated Use oj.common.EmptyRequest.$Properties instead.
         */
        interface IEmptyRequest extends oj.common.EmptyRequest.$Properties {
        }

        /** Represents an EmptyRequest. */
        class EmptyRequest {

            /**
             * Constructs a new EmptyRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.EmptyRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /**
             * Creates a new EmptyRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns EmptyRequest instance
             */
            static create(properties: oj.common.EmptyRequest.$Shape): oj.common.EmptyRequest & oj.common.EmptyRequest.$Shape;
            static create(properties?: oj.common.EmptyRequest.$Properties): oj.common.EmptyRequest;

            /**
             * Encodes the specified EmptyRequest message. Does not implicitly {@link oj.common.EmptyRequest.verify|verify} messages.
             * @param message EmptyRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.EmptyRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified EmptyRequest message, length delimited. Does not implicitly {@link oj.common.EmptyRequest.verify|verify} messages.
             * @param message EmptyRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.EmptyRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an EmptyRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.EmptyRequest & oj.common.EmptyRequest.$Shape} EmptyRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.EmptyRequest & oj.common.EmptyRequest.$Shape;

            /**
             * Decodes an EmptyRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.EmptyRequest & oj.common.EmptyRequest.$Shape} EmptyRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.EmptyRequest & oj.common.EmptyRequest.$Shape;

            /**
             * Verifies an EmptyRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an EmptyRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns EmptyRequest
             */
            static fromObject(object: { [k: string]: any }): oj.common.EmptyRequest;

            /**
             * Creates a plain object from an EmptyRequest message. Also converts values to other types if specified.
             * @param message EmptyRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.EmptyRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this EmptyRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for EmptyRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace EmptyRequest {

            /** Properties of an EmptyRequest. */
            interface $Properties {

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an EmptyRequest. */
            type $Shape = oj.common.EmptyRequest.$Properties;
        }

        /**
         * Properties of an EmptyResponse.
         * @deprecated Use oj.common.EmptyResponse.$Properties instead.
         */
        interface IEmptyResponse extends oj.common.EmptyResponse.$Properties {
        }

        /** Represents an EmptyResponse. */
        class EmptyResponse {

            /**
             * Constructs a new EmptyResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.common.EmptyResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** EmptyResponse status. */
            status?: (oj.common.StatusResponse.$Properties|null);

            /**
             * Creates a new EmptyResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns EmptyResponse instance
             */
            static create(properties: oj.common.EmptyResponse.$Shape): oj.common.EmptyResponse & oj.common.EmptyResponse.$Shape;
            static create(properties?: oj.common.EmptyResponse.$Properties): oj.common.EmptyResponse;

            /**
             * Encodes the specified EmptyResponse message. Does not implicitly {@link oj.common.EmptyResponse.verify|verify} messages.
             * @param message EmptyResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.common.EmptyResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified EmptyResponse message, length delimited. Does not implicitly {@link oj.common.EmptyResponse.verify|verify} messages.
             * @param message EmptyResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.common.EmptyResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an EmptyResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.common.EmptyResponse & oj.common.EmptyResponse.$Shape} EmptyResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.common.EmptyResponse & oj.common.EmptyResponse.$Shape;

            /**
             * Decodes an EmptyResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.common.EmptyResponse & oj.common.EmptyResponse.$Shape} EmptyResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.common.EmptyResponse & oj.common.EmptyResponse.$Shape;

            /**
             * Verifies an EmptyResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an EmptyResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns EmptyResponse
             */
            static fromObject(object: { [k: string]: any }): oj.common.EmptyResponse;

            /**
             * Creates a plain object from an EmptyResponse message. Also converts values to other types if specified.
             * @param message EmptyResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.common.EmptyResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this EmptyResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for EmptyResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace EmptyResponse {

            /** Properties of an EmptyResponse. */
            interface $Properties {

                /** EmptyResponse status */
                status?: (oj.common.StatusResponse.$Properties|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an EmptyResponse. */
            type $Shape = oj.common.EmptyResponse.$Properties;
        }
    }

    /** Namespace gateway. */
    namespace gateway {

        /**
         * Properties of an AuthContext.
         * @deprecated Use oj.gateway.AuthContext.$Properties instead.
         */
        interface IAuthContext extends oj.gateway.AuthContext.$Properties {
        }

        /** Represents an AuthContext. */
        class AuthContext {

            /**
             * Constructs a new AuthContext.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.gateway.AuthContext.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** AuthContext userId. */
            userId: number;

            /** AuthContext email. */
            email: string;

            /** AuthContext role. */
            role: number;

            /** AuthContext jti. */
            jti: string;

            /** AuthContext exp. */
            exp: (Long|string);

            /**
             * Creates a new AuthContext instance using the specified properties.
             * @param [properties] Properties to set
             * @returns AuthContext instance
             */
            static create(properties: oj.gateway.AuthContext.$Shape): oj.gateway.AuthContext & oj.gateway.AuthContext.$Shape;
            static create(properties?: oj.gateway.AuthContext.$Properties): oj.gateway.AuthContext;

            /**
             * Encodes the specified AuthContext message. Does not implicitly {@link oj.gateway.AuthContext.verify|verify} messages.
             * @param message AuthContext message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.gateway.AuthContext.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified AuthContext message, length delimited. Does not implicitly {@link oj.gateway.AuthContext.verify|verify} messages.
             * @param message AuthContext message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.gateway.AuthContext.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes an AuthContext message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.gateway.AuthContext & oj.gateway.AuthContext.$Shape} AuthContext
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.gateway.AuthContext & oj.gateway.AuthContext.$Shape;

            /**
             * Decodes an AuthContext message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.gateway.AuthContext & oj.gateway.AuthContext.$Shape} AuthContext
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.gateway.AuthContext & oj.gateway.AuthContext.$Shape;

            /**
             * Verifies an AuthContext message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates an AuthContext message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns AuthContext
             */
            static fromObject(object: { [k: string]: any }): oj.gateway.AuthContext;

            /**
             * Creates a plain object from an AuthContext message. Also converts values to other types if specified.
             * @param message AuthContext
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.gateway.AuthContext, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this AuthContext to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for AuthContext
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace AuthContext {

            /** Properties of an AuthContext. */
            interface $Properties {

                /** AuthContext userId */
                userId?: (number|null);

                /** AuthContext email */
                email?: (string|null);

                /** AuthContext role */
                role?: (number|null);

                /** AuthContext jti */
                jti?: (string|null);

                /** AuthContext exp */
                exp?: ((Long|string)|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of an AuthContext. */
            type $Shape = oj.gateway.AuthContext.$Properties;
        }

        /**
         * Properties of a RouteEntry.
         * @deprecated Use oj.gateway.RouteEntry.$Properties instead.
         */
        interface IRouteEntry extends oj.gateway.RouteEntry.$Properties {
        }

        /** Represents a RouteEntry. */
        class RouteEntry {

            /**
             * Constructs a new RouteEntry.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.gateway.RouteEntry.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** RouteEntry pathPrefix. */
            pathPrefix: string;

            /** RouteEntry backendService. */
            backendService: string;

            /** RouteEntry backendMethod. */
            backendMethod: string;

            /** RouteEntry requireAuth. */
            requireAuth: boolean;

            /** RouteEntry requireAdmin. */
            requireAdmin: boolean;

            /**
             * Creates a new RouteEntry instance using the specified properties.
             * @param [properties] Properties to set
             * @returns RouteEntry instance
             */
            static create(properties: oj.gateway.RouteEntry.$Shape): oj.gateway.RouteEntry & oj.gateway.RouteEntry.$Shape;
            static create(properties?: oj.gateway.RouteEntry.$Properties): oj.gateway.RouteEntry;

            /**
             * Encodes the specified RouteEntry message. Does not implicitly {@link oj.gateway.RouteEntry.verify|verify} messages.
             * @param message RouteEntry message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.gateway.RouteEntry.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified RouteEntry message, length delimited. Does not implicitly {@link oj.gateway.RouteEntry.verify|verify} messages.
             * @param message RouteEntry message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.gateway.RouteEntry.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a RouteEntry message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.gateway.RouteEntry & oj.gateway.RouteEntry.$Shape} RouteEntry
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.gateway.RouteEntry & oj.gateway.RouteEntry.$Shape;

            /**
             * Decodes a RouteEntry message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.gateway.RouteEntry & oj.gateway.RouteEntry.$Shape} RouteEntry
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.gateway.RouteEntry & oj.gateway.RouteEntry.$Shape;

            /**
             * Verifies a RouteEntry message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a RouteEntry message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns RouteEntry
             */
            static fromObject(object: { [k: string]: any }): oj.gateway.RouteEntry;

            /**
             * Creates a plain object from a RouteEntry message. Also converts values to other types if specified.
             * @param message RouteEntry
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.gateway.RouteEntry, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this RouteEntry to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for RouteEntry
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace RouteEntry {

            /** Properties of a RouteEntry. */
            interface $Properties {

                /** RouteEntry pathPrefix */
                pathPrefix?: (string|null);

                /** RouteEntry backendService */
                backendService?: (string|null);

                /** RouteEntry backendMethod */
                backendMethod?: (string|null);

                /** RouteEntry requireAuth */
                requireAuth?: (boolean|null);

                /** RouteEntry requireAdmin */
                requireAdmin?: (boolean|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a RouteEntry. */
            type $Shape = oj.gateway.RouteEntry.$Properties;
        }

        /**
         * Properties of a RouteTable.
         * @deprecated Use oj.gateway.RouteTable.$Properties instead.
         */
        interface IRouteTable extends oj.gateway.RouteTable.$Properties {
        }

        /** Represents a RouteTable. */
        class RouteTable {

            /**
             * Constructs a new RouteTable.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.gateway.RouteTable.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** RouteTable routes. */
            routes: oj.gateway.RouteEntry.$Properties[];

            /**
             * Creates a new RouteTable instance using the specified properties.
             * @param [properties] Properties to set
             * @returns RouteTable instance
             */
            static create(properties: oj.gateway.RouteTable.$Shape): oj.gateway.RouteTable & oj.gateway.RouteTable.$Shape;
            static create(properties?: oj.gateway.RouteTable.$Properties): oj.gateway.RouteTable;

            /**
             * Encodes the specified RouteTable message. Does not implicitly {@link oj.gateway.RouteTable.verify|verify} messages.
             * @param message RouteTable message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.gateway.RouteTable.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified RouteTable message, length delimited. Does not implicitly {@link oj.gateway.RouteTable.verify|verify} messages.
             * @param message RouteTable message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.gateway.RouteTable.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a RouteTable message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.gateway.RouteTable & oj.gateway.RouteTable.$Shape} RouteTable
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.gateway.RouteTable & oj.gateway.RouteTable.$Shape;

            /**
             * Decodes a RouteTable message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.gateway.RouteTable & oj.gateway.RouteTable.$Shape} RouteTable
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.gateway.RouteTable & oj.gateway.RouteTable.$Shape;

            /**
             * Verifies a RouteTable message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a RouteTable message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns RouteTable
             */
            static fromObject(object: { [k: string]: any }): oj.gateway.RouteTable;

            /**
             * Creates a plain object from a RouteTable message. Also converts values to other types if specified.
             * @param message RouteTable
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.gateway.RouteTable, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this RouteTable to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for RouteTable
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace RouteTable {

            /** Properties of a RouteTable. */
            interface $Properties {

                /** RouteTable routes */
                routes?: (oj.gateway.RouteEntry.$Properties[]|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a RouteTable. */
            type $Shape = oj.gateway.RouteTable.$Properties;
        }

        /**
         * Properties of a PingRequest.
         * @deprecated Use oj.gateway.PingRequest.$Properties instead.
         */
        interface IPingRequest extends oj.gateway.PingRequest.$Properties {
        }

        /** Represents a PingRequest. */
        class PingRequest {

            /**
             * Constructs a new PingRequest.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.gateway.PingRequest.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /**
             * Creates a new PingRequest instance using the specified properties.
             * @param [properties] Properties to set
             * @returns PingRequest instance
             */
            static create(properties: oj.gateway.PingRequest.$Shape): oj.gateway.PingRequest & oj.gateway.PingRequest.$Shape;
            static create(properties?: oj.gateway.PingRequest.$Properties): oj.gateway.PingRequest;

            /**
             * Encodes the specified PingRequest message. Does not implicitly {@link oj.gateway.PingRequest.verify|verify} messages.
             * @param message PingRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.gateway.PingRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified PingRequest message, length delimited. Does not implicitly {@link oj.gateway.PingRequest.verify|verify} messages.
             * @param message PingRequest message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.gateway.PingRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a PingRequest message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.gateway.PingRequest & oj.gateway.PingRequest.$Shape} PingRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.gateway.PingRequest & oj.gateway.PingRequest.$Shape;

            /**
             * Decodes a PingRequest message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.gateway.PingRequest & oj.gateway.PingRequest.$Shape} PingRequest
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.gateway.PingRequest & oj.gateway.PingRequest.$Shape;

            /**
             * Verifies a PingRequest message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a PingRequest message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns PingRequest
             */
            static fromObject(object: { [k: string]: any }): oj.gateway.PingRequest;

            /**
             * Creates a plain object from a PingRequest message. Also converts values to other types if specified.
             * @param message PingRequest
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.gateway.PingRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this PingRequest to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for PingRequest
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace PingRequest {

            /** Properties of a PingRequest. */
            interface $Properties {

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a PingRequest. */
            type $Shape = oj.gateway.PingRequest.$Properties;
        }

        /**
         * Properties of a PingResponse.
         * @deprecated Use oj.gateway.PingResponse.$Properties instead.
         */
        interface IPingResponse extends oj.gateway.PingResponse.$Properties {
        }

        /** Represents a PingResponse. */
        class PingResponse {

            /**
             * Constructs a new PingResponse.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.gateway.PingResponse.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** PingResponse status. */
            status: string;

            /** PingResponse timestamp. */
            timestamp: (Long|string);

            /** PingResponse version. */
            version: string;

            /**
             * Creates a new PingResponse instance using the specified properties.
             * @param [properties] Properties to set
             * @returns PingResponse instance
             */
            static create(properties: oj.gateway.PingResponse.$Shape): oj.gateway.PingResponse & oj.gateway.PingResponse.$Shape;
            static create(properties?: oj.gateway.PingResponse.$Properties): oj.gateway.PingResponse;

            /**
             * Encodes the specified PingResponse message. Does not implicitly {@link oj.gateway.PingResponse.verify|verify} messages.
             * @param message PingResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.gateway.PingResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified PingResponse message, length delimited. Does not implicitly {@link oj.gateway.PingResponse.verify|verify} messages.
             * @param message PingResponse message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.gateway.PingResponse.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a PingResponse message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.gateway.PingResponse & oj.gateway.PingResponse.$Shape} PingResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.gateway.PingResponse & oj.gateway.PingResponse.$Shape;

            /**
             * Decodes a PingResponse message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.gateway.PingResponse & oj.gateway.PingResponse.$Shape} PingResponse
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.gateway.PingResponse & oj.gateway.PingResponse.$Shape;

            /**
             * Verifies a PingResponse message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a PingResponse message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns PingResponse
             */
            static fromObject(object: { [k: string]: any }): oj.gateway.PingResponse;

            /**
             * Creates a plain object from a PingResponse message. Also converts values to other types if specified.
             * @param message PingResponse
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.gateway.PingResponse, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this PingResponse to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for PingResponse
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace PingResponse {

            /** Properties of a PingResponse. */
            interface $Properties {

                /** PingResponse status */
                status?: (string|null);

                /** PingResponse timestamp */
                timestamp?: ((Long|string)|null);

                /** PingResponse version */
                version?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a PingResponse. */
            type $Shape = oj.gateway.PingResponse.$Properties;
        }
    }

    /** Namespace mq. */
    namespace mq {

        /**
         * Properties of a TestCaseInput.
         * @deprecated Use oj.mq.TestCaseInput.$Properties instead.
         */
        interface ITestCaseInput extends oj.mq.TestCaseInput.$Properties {
        }

        /** Represents a TestCaseInput. */
        class TestCaseInput {

            /**
             * Constructs a new TestCaseInput.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.mq.TestCaseInput.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** TestCaseInput testCaseId. */
            testCaseId: (Long|string);

            /** TestCaseInput index. */
            index: number;

            /** TestCaseInput input. */
            input: string;

            /** TestCaseInput expectedOutput. */
            expectedOutput: string;

            /**
             * Creates a new TestCaseInput instance using the specified properties.
             * @param [properties] Properties to set
             * @returns TestCaseInput instance
             */
            static create(properties: oj.mq.TestCaseInput.$Shape): oj.mq.TestCaseInput & oj.mq.TestCaseInput.$Shape;
            static create(properties?: oj.mq.TestCaseInput.$Properties): oj.mq.TestCaseInput;

            /**
             * Encodes the specified TestCaseInput message. Does not implicitly {@link oj.mq.TestCaseInput.verify|verify} messages.
             * @param message TestCaseInput message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.mq.TestCaseInput.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified TestCaseInput message, length delimited. Does not implicitly {@link oj.mq.TestCaseInput.verify|verify} messages.
             * @param message TestCaseInput message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.mq.TestCaseInput.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a TestCaseInput message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.mq.TestCaseInput & oj.mq.TestCaseInput.$Shape} TestCaseInput
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.mq.TestCaseInput & oj.mq.TestCaseInput.$Shape;

            /**
             * Decodes a TestCaseInput message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.mq.TestCaseInput & oj.mq.TestCaseInput.$Shape} TestCaseInput
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.mq.TestCaseInput & oj.mq.TestCaseInput.$Shape;

            /**
             * Verifies a TestCaseInput message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a TestCaseInput message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns TestCaseInput
             */
            static fromObject(object: { [k: string]: any }): oj.mq.TestCaseInput;

            /**
             * Creates a plain object from a TestCaseInput message. Also converts values to other types if specified.
             * @param message TestCaseInput
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.mq.TestCaseInput, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this TestCaseInput to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for TestCaseInput
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace TestCaseInput {

            /** Properties of a TestCaseInput. */
            interface $Properties {

                /** TestCaseInput testCaseId */
                testCaseId?: ((Long|string)|null);

                /** TestCaseInput index */
                index?: (number|null);

                /** TestCaseInput input */
                input?: (string|null);

                /** TestCaseInput expectedOutput */
                expectedOutput?: (string|null);

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Shape of a TestCaseInput. */
            type $Shape = oj.mq.TestCaseInput.$Properties;
        }

        /**
         * Properties of a JudgeTaskMessage.
         * @deprecated Use oj.mq.JudgeTaskMessage.$Properties instead.
         */
        interface IJudgeTaskMessage extends oj.mq.JudgeTaskMessage.$Properties {
        }

        /** Represents a JudgeTaskMessage. */
        class JudgeTaskMessage {

            /**
             * Constructs a new JudgeTaskMessage.
             * @param [properties] Properties to set
             */
            constructor(properties?: oj.mq.JudgeTaskMessage.$Properties);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];

            /** JudgeTaskMessage messageId. */
            messageId: string;

            /** JudgeTaskMessage submissionId. */
            submissionId?: ((Long|string)|null);

            /** JudgeTaskMessage customTaskId. */
            customTaskId?: (string|null);

            /** JudgeTaskMessage userId. */
            userId: number;

            /** JudgeTaskMessage questionId. */
            questionId: string;

            /** JudgeTaskMessage code. */
            code: string;

            /** JudgeTaskMessage language. */
            language: string;

            /** JudgeTaskMessage timeLimitMs. */
            timeLimitMs: number;

            /** JudgeTaskMessage memoryLimitMb. */
            memoryLimitMb: number;

            /** JudgeTaskMessage customInput. */
            customInput: string;

            /** JudgeTaskMessage testCases. */
            testCases: oj.mq.TestCaseInput.$Properties[];

            /** JudgeTaskMessage createdAt. */
            createdAt: (Long|string);

            /** JudgeTaskMessage deliveryAttempt. */
            deliveryAttempt: number;

            /** JudgeTaskMessage taskId. */
            taskId?: ("submissionId"|"customTaskId");

            /**
             * Creates a new JudgeTaskMessage instance using the specified properties.
             * @param [properties] Properties to set
             * @returns JudgeTaskMessage instance
             */
            static create(properties: oj.mq.JudgeTaskMessage.$Shape): oj.mq.JudgeTaskMessage & oj.mq.JudgeTaskMessage.$Shape;
            static create(properties?: oj.mq.JudgeTaskMessage.$Properties): oj.mq.JudgeTaskMessage;

            /**
             * Encodes the specified JudgeTaskMessage message. Does not implicitly {@link oj.mq.JudgeTaskMessage.verify|verify} messages.
             * @param message JudgeTaskMessage message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encode(message: oj.mq.JudgeTaskMessage.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Encodes the specified JudgeTaskMessage message, length delimited. Does not implicitly {@link oj.mq.JudgeTaskMessage.verify|verify} messages.
             * @param message JudgeTaskMessage message or plain object to encode
             * @param [writer] Writer to encode to
             * @returns Writer
             */
            static encodeDelimited(message: oj.mq.JudgeTaskMessage.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

            /**
             * Decodes a JudgeTaskMessage message from the specified reader or buffer.
             * @param reader Reader or buffer to decode from
             * @param [length] Message length if known beforehand
             * @returns {oj.mq.JudgeTaskMessage & oj.mq.JudgeTaskMessage.$Shape} JudgeTaskMessage
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj.mq.JudgeTaskMessage & oj.mq.JudgeTaskMessage.$Shape;

            /**
             * Decodes a JudgeTaskMessage message from the specified reader or buffer, length delimited.
             * @param reader Reader or buffer to decode from
             * @returns {oj.mq.JudgeTaskMessage & oj.mq.JudgeTaskMessage.$Shape} JudgeTaskMessage
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj.mq.JudgeTaskMessage & oj.mq.JudgeTaskMessage.$Shape;

            /**
             * Verifies a JudgeTaskMessage message.
             * @param message Plain object to verify
             * @returns `null` if valid, otherwise the reason why it is not
             */
            static verify(message: { [k: string]: any }): (string|null);

            /**
             * Creates a JudgeTaskMessage message from a plain object. Also converts values to their respective internal types.
             * @param object Plain object
             * @returns JudgeTaskMessage
             */
            static fromObject(object: { [k: string]: any }): oj.mq.JudgeTaskMessage;

            /**
             * Creates a plain object from a JudgeTaskMessage message. Also converts values to other types if specified.
             * @param message JudgeTaskMessage
             * @param [options] Conversion options
             * @returns Plain object
             */
            static toObject(message: oj.mq.JudgeTaskMessage, options?: $protobuf.IConversionOptions): { [k: string]: any };

            /**
             * Converts this JudgeTaskMessage to JSON.
             * @returns JSON object
             */
            toJSON(): { [k: string]: any };

            /**
             * Gets the type url for JudgeTaskMessage
             * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
             * @returns The type url
             */
            static getTypeUrl(prefix?: string): string;
        }

        namespace JudgeTaskMessage {

            /** Properties of a JudgeTaskMessage. */
            interface $Properties {

                /** JudgeTaskMessage messageId */
                messageId?: (string|null);

                /** JudgeTaskMessage submissionId */
                submissionId?: ((Long|string)|null);

                /** JudgeTaskMessage customTaskId */
                customTaskId?: (string|null);

                /** JudgeTaskMessage userId */
                userId?: (number|null);

                /** JudgeTaskMessage questionId */
                questionId?: (string|null);

                /** JudgeTaskMessage code */
                code?: (string|null);

                /** JudgeTaskMessage language */
                language?: (string|null);

                /** JudgeTaskMessage timeLimitMs */
                timeLimitMs?: (number|null);

                /** JudgeTaskMessage memoryLimitMb */
                memoryLimitMb?: (number|null);

                /** JudgeTaskMessage customInput */
                customInput?: (string|null);

                /** JudgeTaskMessage testCases */
                testCases?: (oj.mq.TestCaseInput.$Properties[]|null);

                /** JudgeTaskMessage createdAt */
                createdAt?: ((Long|string)|null);

                /** JudgeTaskMessage deliveryAttempt */
                deliveryAttempt?: (number|null);

                /** JudgeTaskMessage taskId */
                taskId?: ("submissionId"|"customTaskId");

                /** Unknown fields preserved while decoding when enabled */
                $unknowns?: Uint8Array[];
            }

            /** Narrowed shape of a JudgeTaskMessage. */
            type $Shape = {
              messageId?: string|null;
              submissionId?: (Long|string)|null;
              customTaskId?: string|null;
              userId?: number|null;
              questionId?: string|null;
              code?: string|null;
              language?: string|null;
              timeLimitMs?: number|null;
              memoryLimitMb?: number|null;
              customInput?: string|null;
              testCases?: oj.mq.TestCaseInput.$Shape[]|null;
              createdAt?: (Long|string)|null;
              deliveryAttempt?: number|null;
              $unknowns?: Uint8Array[];
            } & (
              ({ taskId?: undefined; submissionId?: null; customTaskId?: null }|{ taskId?: "submissionId"; submissionId: (Long|string); customTaskId?: null }|{ taskId?: "customTaskId"; submissionId?: null; customTaskId: string })
            );
        }
    }
}

/** Namespace oj_judge. */
export namespace oj_judge {

    /**
     * Properties of a SubmitRequest.
     * @deprecated Use oj_judge.SubmitRequest.$Properties instead.
     */
    interface ISubmitRequest extends oj_judge.SubmitRequest.$Properties {
    }

    /** Represents a SubmitRequest. */
    class SubmitRequest {

        /**
         * Constructs a new SubmitRequest.
         * @param [properties] Properties to set
         */
        constructor(properties?: oj_judge.SubmitRequest.$Properties);

        /** Unknown fields preserved while decoding when enabled */
        $unknowns?: Uint8Array[];

        /** SubmitRequest submissionId. */
        submissionId: number;

        /** SubmitRequest questionId. */
        questionId: string;

        /** SubmitRequest userId. */
        userId: number;

        /** SubmitRequest code. */
        code: string;

        /** SubmitRequest timestamp. */
        timestamp: (Long|string);

        /** SubmitRequest language. */
        language: string;

        /** SubmitRequest timeLimit. */
        timeLimit: number;

        /** SubmitRequest memoryLimit. */
        memoryLimit: number;

        /** SubmitRequest isCustomTest. */
        isCustomTest: boolean;

        /** SubmitRequest customInput. */
        customInput: string;

        /**
         * Creates a new SubmitRequest instance using the specified properties.
         * @param [properties] Properties to set
         * @returns SubmitRequest instance
         */
        static create(properties: oj_judge.SubmitRequest.$Shape): oj_judge.SubmitRequest & oj_judge.SubmitRequest.$Shape;
        static create(properties?: oj_judge.SubmitRequest.$Properties): oj_judge.SubmitRequest;

        /**
         * Encodes the specified SubmitRequest message. Does not implicitly {@link oj_judge.SubmitRequest.verify|verify} messages.
         * @param message SubmitRequest message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encode(message: oj_judge.SubmitRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Encodes the specified SubmitRequest message, length delimited. Does not implicitly {@link oj_judge.SubmitRequest.verify|verify} messages.
         * @param message SubmitRequest message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encodeDelimited(message: oj_judge.SubmitRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Decodes a SubmitRequest message from the specified reader or buffer.
         * @param reader Reader or buffer to decode from
         * @param [length] Message length if known beforehand
         * @returns {oj_judge.SubmitRequest & oj_judge.SubmitRequest.$Shape} SubmitRequest
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj_judge.SubmitRequest & oj_judge.SubmitRequest.$Shape;

        /**
         * Decodes a SubmitRequest message from the specified reader or buffer, length delimited.
         * @param reader Reader or buffer to decode from
         * @returns {oj_judge.SubmitRequest & oj_judge.SubmitRequest.$Shape} SubmitRequest
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj_judge.SubmitRequest & oj_judge.SubmitRequest.$Shape;

        /**
         * Verifies a SubmitRequest message.
         * @param message Plain object to verify
         * @returns `null` if valid, otherwise the reason why it is not
         */
        static verify(message: { [k: string]: any }): (string|null);

        /**
         * Creates a SubmitRequest message from a plain object. Also converts values to their respective internal types.
         * @param object Plain object
         * @returns SubmitRequest
         */
        static fromObject(object: { [k: string]: any }): oj_judge.SubmitRequest;

        /**
         * Creates a plain object from a SubmitRequest message. Also converts values to other types if specified.
         * @param message SubmitRequest
         * @param [options] Conversion options
         * @returns Plain object
         */
        static toObject(message: oj_judge.SubmitRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

        /**
         * Converts this SubmitRequest to JSON.
         * @returns JSON object
         */
        toJSON(): { [k: string]: any };

        /**
         * Gets the type url for SubmitRequest
         * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
         * @returns The type url
         */
        static getTypeUrl(prefix?: string): string;
    }

    namespace SubmitRequest {

        /** Properties of a SubmitRequest. */
        interface $Properties {

            /** SubmitRequest submissionId */
            submissionId?: (number|null);

            /** SubmitRequest questionId */
            questionId?: (string|null);

            /** SubmitRequest userId */
            userId?: (number|null);

            /** SubmitRequest code */
            code?: (string|null);

            /** SubmitRequest timestamp */
            timestamp?: ((Long|string)|null);

            /** SubmitRequest language */
            language?: (string|null);

            /** SubmitRequest timeLimit */
            timeLimit?: (number|null);

            /** SubmitRequest memoryLimit */
            memoryLimit?: (number|null);

            /** SubmitRequest isCustomTest */
            isCustomTest?: (boolean|null);

            /** SubmitRequest customInput */
            customInput?: (string|null);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];
        }

        /** Shape of a SubmitRequest. */
        type $Shape = oj_judge.SubmitRequest.$Properties;
    }

    /** SubmitResult enum. */
    enum SubmitResult {

        /** AC value */
        AC = 0,

        /** WA value */
        WA = 1,

        /** COMPILE_ERROR value */
        COMPILE_ERROR = 2,

        /** MEMORY_LIMIT value */
        MEMORY_LIMIT = 3,

        /** TIME_LIMIT value */
        TIME_LIMIT = 4,

        /** SEGV_ERROR value */
        SEGV_ERROR = 5,

        /** FPE_ERROR value */
        FPE_ERROR = 6,

        /** RUNTIME_ERROR value */
        RUNTIME_ERROR = 7,

        /** UNKNOWN value */
        UNKNOWN = 8
    }

    /**
     * Properties of a SubmitStatus.
     * @deprecated Use oj_judge.SubmitStatus.$Properties instead.
     */
    interface ISubmitStatus extends oj_judge.SubmitStatus.$Properties {
    }

    /** Represents a SubmitStatus. */
    class SubmitStatus {

        /**
         * Constructs a new SubmitStatus.
         * @param [properties] Properties to set
         */
        constructor(properties?: oj_judge.SubmitStatus.$Properties);

        /** Unknown fields preserved while decoding when enabled */
        $unknowns?: Uint8Array[];

        /** SubmitStatus timeCost. */
        timeCost: number;

        /** SubmitStatus memoryCost. */
        memoryCost: number;

        /** SubmitStatus result. */
        result: oj_judge.SubmitResult;

        /**
         * Creates a new SubmitStatus instance using the specified properties.
         * @param [properties] Properties to set
         * @returns SubmitStatus instance
         */
        static create(properties: oj_judge.SubmitStatus.$Shape): oj_judge.SubmitStatus & oj_judge.SubmitStatus.$Shape;
        static create(properties?: oj_judge.SubmitStatus.$Properties): oj_judge.SubmitStatus;

        /**
         * Encodes the specified SubmitStatus message. Does not implicitly {@link oj_judge.SubmitStatus.verify|verify} messages.
         * @param message SubmitStatus message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encode(message: oj_judge.SubmitStatus.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Encodes the specified SubmitStatus message, length delimited. Does not implicitly {@link oj_judge.SubmitStatus.verify|verify} messages.
         * @param message SubmitStatus message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encodeDelimited(message: oj_judge.SubmitStatus.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Decodes a SubmitStatus message from the specified reader or buffer.
         * @param reader Reader or buffer to decode from
         * @param [length] Message length if known beforehand
         * @returns {oj_judge.SubmitStatus & oj_judge.SubmitStatus.$Shape} SubmitStatus
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj_judge.SubmitStatus & oj_judge.SubmitStatus.$Shape;

        /**
         * Decodes a SubmitStatus message from the specified reader or buffer, length delimited.
         * @param reader Reader or buffer to decode from
         * @returns {oj_judge.SubmitStatus & oj_judge.SubmitStatus.$Shape} SubmitStatus
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj_judge.SubmitStatus & oj_judge.SubmitStatus.$Shape;

        /**
         * Verifies a SubmitStatus message.
         * @param message Plain object to verify
         * @returns `null` if valid, otherwise the reason why it is not
         */
        static verify(message: { [k: string]: any }): (string|null);

        /**
         * Creates a SubmitStatus message from a plain object. Also converts values to their respective internal types.
         * @param object Plain object
         * @returns SubmitStatus
         */
        static fromObject(object: { [k: string]: any }): oj_judge.SubmitStatus;

        /**
         * Creates a plain object from a SubmitStatus message. Also converts values to other types if specified.
         * @param message SubmitStatus
         * @param [options] Conversion options
         * @returns Plain object
         */
        static toObject(message: oj_judge.SubmitStatus, options?: $protobuf.IConversionOptions): { [k: string]: any };

        /**
         * Converts this SubmitStatus to JSON.
         * @returns JSON object
         */
        toJSON(): { [k: string]: any };

        /**
         * Gets the type url for SubmitStatus
         * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
         * @returns The type url
         */
        static getTypeUrl(prefix?: string): string;
    }

    namespace SubmitStatus {

        /** Properties of a SubmitStatus. */
        interface $Properties {

            /** SubmitStatus timeCost */
            timeCost?: (number|null);

            /** SubmitStatus memoryCost */
            memoryCost?: (number|null);

            /** SubmitStatus result */
            result?: (oj_judge.SubmitResult|null);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];
        }

        /** Shape of a SubmitStatus. */
        type $Shape = oj_judge.SubmitStatus.$Properties;
    }

    /**
     * Properties of a JudgeFinishedRequest.
     * @deprecated Use oj_judge.JudgeFinishedRequest.$Properties instead.
     */
    interface IJudgeFinishedRequest extends oj_judge.JudgeFinishedRequest.$Properties {
    }

    /** Represents a JudgeFinishedRequest. */
    class JudgeFinishedRequest {

        /**
         * Constructs a new JudgeFinishedRequest.
         * @param [properties] Properties to set
         */
        constructor(properties?: oj_judge.JudgeFinishedRequest.$Properties);

        /** Unknown fields preserved while decoding when enabled */
        $unknowns?: Uint8Array[];

        /** JudgeFinishedRequest submissionId. */
        submissionId: number;

        /** JudgeFinishedRequest questionId. */
        questionId: string;

        /** JudgeFinishedRequest userId. */
        userId: number;

        /** JudgeFinishedRequest statusList. */
        statusList: oj_judge.SubmitStatus.$Properties[];

        /**
         * Creates a new JudgeFinishedRequest instance using the specified properties.
         * @param [properties] Properties to set
         * @returns JudgeFinishedRequest instance
         */
        static create(properties: oj_judge.JudgeFinishedRequest.$Shape): oj_judge.JudgeFinishedRequest & oj_judge.JudgeFinishedRequest.$Shape;
        static create(properties?: oj_judge.JudgeFinishedRequest.$Properties): oj_judge.JudgeFinishedRequest;

        /**
         * Encodes the specified JudgeFinishedRequest message. Does not implicitly {@link oj_judge.JudgeFinishedRequest.verify|verify} messages.
         * @param message JudgeFinishedRequest message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encode(message: oj_judge.JudgeFinishedRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Encodes the specified JudgeFinishedRequest message, length delimited. Does not implicitly {@link oj_judge.JudgeFinishedRequest.verify|verify} messages.
         * @param message JudgeFinishedRequest message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encodeDelimited(message: oj_judge.JudgeFinishedRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Decodes a JudgeFinishedRequest message from the specified reader or buffer.
         * @param reader Reader or buffer to decode from
         * @param [length] Message length if known beforehand
         * @returns {oj_judge.JudgeFinishedRequest & oj_judge.JudgeFinishedRequest.$Shape} JudgeFinishedRequest
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj_judge.JudgeFinishedRequest & oj_judge.JudgeFinishedRequest.$Shape;

        /**
         * Decodes a JudgeFinishedRequest message from the specified reader or buffer, length delimited.
         * @param reader Reader or buffer to decode from
         * @returns {oj_judge.JudgeFinishedRequest & oj_judge.JudgeFinishedRequest.$Shape} JudgeFinishedRequest
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj_judge.JudgeFinishedRequest & oj_judge.JudgeFinishedRequest.$Shape;

        /**
         * Verifies a JudgeFinishedRequest message.
         * @param message Plain object to verify
         * @returns `null` if valid, otherwise the reason why it is not
         */
        static verify(message: { [k: string]: any }): (string|null);

        /**
         * Creates a JudgeFinishedRequest message from a plain object. Also converts values to their respective internal types.
         * @param object Plain object
         * @returns JudgeFinishedRequest
         */
        static fromObject(object: { [k: string]: any }): oj_judge.JudgeFinishedRequest;

        /**
         * Creates a plain object from a JudgeFinishedRequest message. Also converts values to other types if specified.
         * @param message JudgeFinishedRequest
         * @param [options] Conversion options
         * @returns Plain object
         */
        static toObject(message: oj_judge.JudgeFinishedRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

        /**
         * Converts this JudgeFinishedRequest to JSON.
         * @returns JSON object
         */
        toJSON(): { [k: string]: any };

        /**
         * Gets the type url for JudgeFinishedRequest
         * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
         * @returns The type url
         */
        static getTypeUrl(prefix?: string): string;
    }

    namespace JudgeFinishedRequest {

        /** Properties of a JudgeFinishedRequest. */
        interface $Properties {

            /** JudgeFinishedRequest submissionId */
            submissionId?: (number|null);

            /** JudgeFinishedRequest questionId */
            questionId?: (string|null);

            /** JudgeFinishedRequest userId */
            userId?: (number|null);

            /** JudgeFinishedRequest statusList */
            statusList?: (oj_judge.SubmitStatus.$Properties[]|null);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];
        }

        /** Shape of a JudgeFinishedRequest. */
        type $Shape = oj_judge.JudgeFinishedRequest.$Properties;
    }

    /**
     * Properties of a DockerWorkDoneRequest.
     * @deprecated Use oj_judge.DockerWorkDoneRequest.$Properties instead.
     */
    interface IDockerWorkDoneRequest extends oj_judge.DockerWorkDoneRequest.$Properties {
    }

    /** Represents a DockerWorkDoneRequest. */
    class DockerWorkDoneRequest {

        /**
         * Constructs a new DockerWorkDoneRequest.
         * @param [properties] Properties to set
         */
        constructor(properties?: oj_judge.DockerWorkDoneRequest.$Properties);

        /** Unknown fields preserved while decoding when enabled */
        $unknowns?: Uint8Array[];

        /** DockerWorkDoneRequest id. */
        id: string;

        /** DockerWorkDoneRequest status. */
        status: string;

        /** DockerWorkDoneRequest exitCode. */
        exitCode: number;

        /**
         * Creates a new DockerWorkDoneRequest instance using the specified properties.
         * @param [properties] Properties to set
         * @returns DockerWorkDoneRequest instance
         */
        static create(properties: oj_judge.DockerWorkDoneRequest.$Shape): oj_judge.DockerWorkDoneRequest & oj_judge.DockerWorkDoneRequest.$Shape;
        static create(properties?: oj_judge.DockerWorkDoneRequest.$Properties): oj_judge.DockerWorkDoneRequest;

        /**
         * Encodes the specified DockerWorkDoneRequest message. Does not implicitly {@link oj_judge.DockerWorkDoneRequest.verify|verify} messages.
         * @param message DockerWorkDoneRequest message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encode(message: oj_judge.DockerWorkDoneRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Encodes the specified DockerWorkDoneRequest message, length delimited. Does not implicitly {@link oj_judge.DockerWorkDoneRequest.verify|verify} messages.
         * @param message DockerWorkDoneRequest message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encodeDelimited(message: oj_judge.DockerWorkDoneRequest.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Decodes a DockerWorkDoneRequest message from the specified reader or buffer.
         * @param reader Reader or buffer to decode from
         * @param [length] Message length if known beforehand
         * @returns {oj_judge.DockerWorkDoneRequest & oj_judge.DockerWorkDoneRequest.$Shape} DockerWorkDoneRequest
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj_judge.DockerWorkDoneRequest & oj_judge.DockerWorkDoneRequest.$Shape;

        /**
         * Decodes a DockerWorkDoneRequest message from the specified reader or buffer, length delimited.
         * @param reader Reader or buffer to decode from
         * @returns {oj_judge.DockerWorkDoneRequest & oj_judge.DockerWorkDoneRequest.$Shape} DockerWorkDoneRequest
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj_judge.DockerWorkDoneRequest & oj_judge.DockerWorkDoneRequest.$Shape;

        /**
         * Verifies a DockerWorkDoneRequest message.
         * @param message Plain object to verify
         * @returns `null` if valid, otherwise the reason why it is not
         */
        static verify(message: { [k: string]: any }): (string|null);

        /**
         * Creates a DockerWorkDoneRequest message from a plain object. Also converts values to their respective internal types.
         * @param object Plain object
         * @returns DockerWorkDoneRequest
         */
        static fromObject(object: { [k: string]: any }): oj_judge.DockerWorkDoneRequest;

        /**
         * Creates a plain object from a DockerWorkDoneRequest message. Also converts values to other types if specified.
         * @param message DockerWorkDoneRequest
         * @param [options] Conversion options
         * @returns Plain object
         */
        static toObject(message: oj_judge.DockerWorkDoneRequest, options?: $protobuf.IConversionOptions): { [k: string]: any };

        /**
         * Converts this DockerWorkDoneRequest to JSON.
         * @returns JSON object
         */
        toJSON(): { [k: string]: any };

        /**
         * Gets the type url for DockerWorkDoneRequest
         * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
         * @returns The type url
         */
        static getTypeUrl(prefix?: string): string;
    }

    namespace DockerWorkDoneRequest {

        /** Properties of a DockerWorkDoneRequest. */
        interface $Properties {

            /** DockerWorkDoneRequest id */
            id?: (string|null);

            /** DockerWorkDoneRequest status */
            status?: (string|null);

            /** DockerWorkDoneRequest exitCode */
            exitCode?: (number|null);

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];
        }

        /** Shape of a DockerWorkDoneRequest. */
        type $Shape = oj_judge.DockerWorkDoneRequest.$Properties;
    }

    /**
     * Properties of a NullRsp.
     * @deprecated Use oj_judge.NullRsp.$Properties instead.
     */
    interface INullRsp extends oj_judge.NullRsp.$Properties {
    }

    /** Represents a NullRsp. */
    class NullRsp {

        /**
         * Constructs a new NullRsp.
         * @param [properties] Properties to set
         */
        constructor(properties?: oj_judge.NullRsp.$Properties);

        /** Unknown fields preserved while decoding when enabled */
        $unknowns?: Uint8Array[];

        /**
         * Creates a new NullRsp instance using the specified properties.
         * @param [properties] Properties to set
         * @returns NullRsp instance
         */
        static create(properties: oj_judge.NullRsp.$Shape): oj_judge.NullRsp & oj_judge.NullRsp.$Shape;
        static create(properties?: oj_judge.NullRsp.$Properties): oj_judge.NullRsp;

        /**
         * Encodes the specified NullRsp message. Does not implicitly {@link oj_judge.NullRsp.verify|verify} messages.
         * @param message NullRsp message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encode(message: oj_judge.NullRsp.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Encodes the specified NullRsp message, length delimited. Does not implicitly {@link oj_judge.NullRsp.verify|verify} messages.
         * @param message NullRsp message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encodeDelimited(message: oj_judge.NullRsp.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Decodes a NullRsp message from the specified reader or buffer.
         * @param reader Reader or buffer to decode from
         * @param [length] Message length if known beforehand
         * @returns {oj_judge.NullRsp & oj_judge.NullRsp.$Shape} NullRsp
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): oj_judge.NullRsp & oj_judge.NullRsp.$Shape;

        /**
         * Decodes a NullRsp message from the specified reader or buffer, length delimited.
         * @param reader Reader or buffer to decode from
         * @returns {oj_judge.NullRsp & oj_judge.NullRsp.$Shape} NullRsp
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): oj_judge.NullRsp & oj_judge.NullRsp.$Shape;

        /**
         * Verifies a NullRsp message.
         * @param message Plain object to verify
         * @returns `null` if valid, otherwise the reason why it is not
         */
        static verify(message: { [k: string]: any }): (string|null);

        /**
         * Creates a NullRsp message from a plain object. Also converts values to their respective internal types.
         * @param object Plain object
         * @returns NullRsp
         */
        static fromObject(object: { [k: string]: any }): oj_judge.NullRsp;

        /**
         * Creates a plain object from a NullRsp message. Also converts values to other types if specified.
         * @param message NullRsp
         * @param [options] Conversion options
         * @returns Plain object
         */
        static toObject(message: oj_judge.NullRsp, options?: $protobuf.IConversionOptions): { [k: string]: any };

        /**
         * Converts this NullRsp to JSON.
         * @returns JSON object
         */
        toJSON(): { [k: string]: any };

        /**
         * Gets the type url for NullRsp
         * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
         * @returns The type url
         */
        static getTypeUrl(prefix?: string): string;
    }

    namespace NullRsp {

        /** Properties of a NullRsp. */
        interface $Properties {

            /** Unknown fields preserved while decoding when enabled */
            $unknowns?: Uint8Array[];
        }

        /** Shape of a NullRsp. */
        type $Shape = oj_judge.NullRsp.$Properties;
    }

    /** Represents a JudgeService */
    class JudgeService extends $protobuf.rpc.Service {

        /**
         * Constructs a new JudgeService service.
         * @param rpcImpl RPC implementation
         * @param [requestDelimited=false] Whether requests are length-delimited
         * @param [responseDelimited=false] Whether responses are length-delimited
         */
        constructor(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean);

        /**
         * Creates new JudgeService service using the specified rpc implementation.
         * @param rpcImpl RPC implementation
         * @param [requestDelimited=false] Whether requests are length-delimited
         * @param [responseDelimited=false] Whether responses are length-delimited
         * @returns RPC service. Useful where requests and/or responses are streamed.
         */
        static create(rpcImpl: $protobuf.RPCImpl, requestDelimited?: boolean, responseDelimited?: boolean): JudgeService;

        /** Calls DockerWorkDone. */
        dockerWorkDone: oj_judge.JudgeService.DockerWorkDone;
    }

    namespace JudgeService {

        /**
         * Callback as used by {@link oj_judge.JudgeService#dockerWorkDone}.
         * @param error Error, if any
         * @param [response] NullRsp
         */
        type DockerWorkDoneCallback = (error: (Error|null), response?: oj_judge.NullRsp) => void;

        /** Calls DockerWorkDone. */
        type DockerWorkDone = {
          (request: oj_judge.IDockerWorkDoneRequest, callback: oj_judge.JudgeService.DockerWorkDoneCallback): void;
          (request: oj_judge.IDockerWorkDoneRequest): Promise<oj_judge.NullRsp>;
          readonly name: "DockerWorkDone";
          readonly path: "/oj_judge.JudgeService/DockerWorkDone";
          readonly requestType: "DockerWorkDoneRequest";
          readonly responseType: "NullRsp";
          readonly requestStream: undefined;
          readonly responseStream: undefined;
        };
    }
}
