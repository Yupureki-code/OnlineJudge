local http = require "resty.http"

local M = {}

local function gateway_token()
    local path = os.getenv("OJ_GATEWAY_AUTH_TOKEN_FILE")
    if path then
        local file = io.open(path, "r")
        if file then
            local value = file:read("*l")
            file:close()
            if value and value ~= "" then return value end
        end
    end
    return os.getenv("OJ_GATEWAY_AUTH_TOKEN")
end

local function classify_error(request_error)
    if request_error and request_error:lower():find("timeout", 1, true) then
        return "UPSTREAM_TIMEOUT"
    end
    return "UPSTREAM_UNAVAILABLE"
end

local function upstream_for(service)
    if service == "oj.biz.OJAdminService" then
        return os.getenv("OJ_ADMIN_UPSTREAM") or "http://oj_admin:8090"
    end
    return os.getenv("OJ_SERVER_UPSTREAM") or "http://oj_server:8080"
end

local function cookie_value(cookie_header, wanted_name)
    if not cookie_header then return nil end
    for item in cookie_header:gmatch("[^;]+") do
        local name, value = item:match("^%s*([^=%s]+)%s*=%s*(.-)%s*$")
        if name == wanted_name and value ~= "" then
            return value
        end
    end
    return nil
end

local function service_cookie(service, cookie_header)
    local name = service == "oj.biz.OJAdminService" and "oj_admin_session_id" or "oj_session_id"
    local value = cookie_value(cookie_header, name)
    return value and (name .. "=" .. value) or nil
end

local function forwarded_headers(route)
    local headers = {
        ["Content-Type"] = "application/proto",
        ["Accept"] = "application/proto",
        ["Cookie"] = service_cookie(route.service, ngx.var.http_cookie),
        ["X-Request-Id"] = ngx.var.http_x_request_id or ngx.var.request_id,
        ["X-OJ-Client-IP"] = ngx.var.remote_addr,
        ["X-OJ-Gateway-Token"] = gateway_token(),
    }
    for _, name in ipairs({"traceparent", "tracestate", "baggage", "x-b3-traceid", "x-b3-spanid"}) do
        local value = ngx.req.get_headers()[name]
        if value then headers[name] = value end
    end
    return headers
end

function M.call(route, body)
    local client = http.new()
    local started_at = ngx.now()
    client:set_timeouts(
        tonumber(os.getenv("OJ_UPSTREAM_CONNECT_TIMEOUT_MS")) or 1000,
        tonumber(os.getenv("OJ_UPSTREAM_SEND_TIMEOUT_MS")) or 2000,
        tonumber(os.getenv("OJ_UPSTREAM_READ_TIMEOUT_MS")) or 5000)
    local url = upstream_for(route.service) .. "/" .. route.service .. "/" .. route.method
    local response, request_error = client:request_uri(url, {
        method = "POST",
        body = body,
        headers = forwarded_headers(route),
        keepalive = true,
        keepalive_timeout = tonumber(os.getenv("OJ_UPSTREAM_KEEPALIVE_TIMEOUT_MS")) or 30000,
        keepalive_pool = tonumber(os.getenv("OJ_UPSTREAM_KEEPALIVE_POOL")) or 32,
    })
    if not response then
        return nil, classify_error(request_error), request_error
    end
    local total_timeout = tonumber(os.getenv("OJ_UPSTREAM_TOTAL_TIMEOUT_MS")) or 8000
    if (ngx.now() - started_at) * 1000 > total_timeout then
        return nil, "UPSTREAM_TIMEOUT", "total upstream timeout exceeded"
    end
    local maximum = tonumber(os.getenv("OJ_GATEWAY_MAX_UPSTREAM_RESPONSE")) or 4194304
    if not response.body or #response.body > maximum then
        return nil, "INVALID_UPSTREAM_RESPONSE", "response body exceeds limit"
    end
    return response
end

M.classify_error = classify_error
M.service_cookie = service_cookie
M.forwarded_headers = forwarded_headers
return M
