local csrf = require "csrf"

local M = {}

local function split(value)
    local result = {}
    for item in (value or ""):gmatch("[^,]+") do
        item = item:match("^%s*(.-)%s*$")
        if item ~= "" then result[item] = true end
    end
    return result
end

local function origin_from_referer(referer)
    if not referer then return nil end
    return referer:match("^(https?://[^/]+)")
end

local function has_cookie(name)
    local header = ngx.var.http_cookie or ""
    for key in header:gmatch("%s*([^=;]+)=") do
        if key == name then return true end
    end
    return false
end

function M.validate(route)
    local uri = ngx.var.request_uri or ""
    if uri:find("\\", 1, true) or uri:find("%%2[fF]") or uri:find("%%5[cC]") or
       uri:find("/../", 1, true) or uri:find("//", 1, true) then
        return nil, 400, "PATH_CONFUSION"
    end
    if ngx.var.http_transfer_encoding then return nil, 400, "TRANSFER_ENCODING_UNSUPPORTED" end
    local encoding = ngx.var.http_content_encoding
    if encoding and encoding:lower() ~= "identity" then return nil, 415, "CONTENT_ENCODING_UNSUPPORTED" end
    if (ngx.req.get_method() == "GET" or ngx.req.get_method() == "HEAD") and
       tonumber(ngx.var.content_length or "0") > 0 then
        return nil, 400, "BODY_NOT_ALLOWED"
    end

    local auth = route.auth
    if auth == "user" and not has_cookie("oj_session_id") then return nil, 401, "UNAUTHORIZED" end
    if (auth == "admin" or auth == "super_admin") and not has_cookie("oj_admin_session_id") then
        return nil, 401, "ADMIN_UNAUTHORIZED"
    end

    if route.csrf then
        local allowed = split(os.getenv("OJ_ALLOWED_ORIGINS") or "https://localhost")
        local supplied = ngx.var.http_origin or origin_from_referer(ngx.var.http_referer)
        if not supplied or not allowed[supplied] then return nil, 403, "ORIGIN_FORBIDDEN" end
        local ok, csrf_error = csrf.verify()
        if not ok then return nil, 403, csrf_error end
    end
    return true
end

function M.read_json(route)
    if ngx.req.get_method() == "GET" or ngx.req.get_method() == "HEAD" then return {} end
    local content_type = ngx.var.content_type
    if not content_type or not content_type:lower():match("^application/json%s*;?") then
        return nil, 415, "JSON_CONTENT_TYPE_REQUIRED"
    end
    ngx.req.read_body()
    local body = ngx.req.get_body_data()
    if not body then
        if ngx.req.get_body_file() then return nil, 413, "JSON_BODY_TOO_LARGE" end
        return {}
    end
    local maximum = tonumber(os.getenv("OJ_GATEWAY_MAX_JSON_BODY")) or 1048576
    if #body > maximum then return nil, 413, "JSON_BODY_TOO_LARGE" end
    if body == "" then return {} end
    local cjson = require "cjson.safe"
    local decoded, decode_error = cjson.decode(body)
    if type(decoded) ~= "table" then return nil, 400, "MALFORMED_JSON", decode_error end
    if route.method == "UpdateAvatar" then
        local max_avatar = tonumber(os.getenv("OJ_GATEWAY_MAX_AVATAR_BASE64")) or 2097152
        if type(decoded.content) == "string" and #decoded.content > max_avatar then
            return nil, 413, "AVATAR_TOO_LARGE"
        end
    end
    local max_code = tonumber(os.getenv("OJ_GATEWAY_MAX_CODE")) or 65536
    if type(decoded.code) == "string" and #decoded.code > max_code then return nil, 413, "CODE_TOO_LARGE" end
    local max_input = tonumber(os.getenv("OJ_GATEWAY_MAX_CUSTOM_INPUT")) or 65536
    if type(decoded.custom_input) == "string" and #decoded.custom_input > max_input then
        return nil, 413, "CUSTOM_INPUT_TOO_LARGE"
    end
    return decoded
end

return M
