local routes = require "routes"
local codec = require "protobuf_codec"
local upstream = require "brpc_upstream"
local policy = require "request_policy"
local csrf = require "csrf"
local errors = require "error_response"

local M = {}
local compiled = nil

local function escape_pattern(value)
    return (value:gsub("([%%%^%$%(%)%.%[%]%*%+%-%?])", "%%%1"))
end

local function compile()
    if compiled then return compiled end
    compiled = {}
    local seen = {}
    for _, route in ipairs(routes) do
        local names = {}
        local pattern = "^"
        for segment in route.path:gmatch("/([^/]+)") do
            pattern = pattern .. "/"
            local name = segment:match("^:(.+)$")
            if name then
                names[#names + 1] = name
                pattern = pattern .. "([^/]+)"
            else
                pattern = pattern .. escape_pattern(segment)
            end
        end
        if route.path == "/" then pattern = "^/" end
        pattern = pattern .. "$"
        local key = route.verb .. " " .. route.path
        if seen[key] then error("duplicate gateway route: " .. key) end
        seen[key] = true
        compiled[#compiled + 1] = { route = route, pattern = pattern, names = names }
    end
    return compiled
end

local function find_route(method, uri)
    for _, item in ipairs(compile()) do
        if item.route.verb == method then
            local captures = { uri:match(item.pattern) }
            if #captures > 0 or uri == item.route.path then
                local parameters = {}
                for index, name in ipairs(item.names) do parameters[name] = ngx.unescape_uri(captures[index]) end
                return item.route, parameters
            end
        end
    end
end

local function set_path(target, dotted, value)
    local current = target
    local parts = {}
    for part in dotted:gmatch("[^.]+") do parts[#parts + 1] = part end
    for index = 1, #parts - 1 do
        local part = parts[index]
        if current[part] == nil then current[part] = {} end
        if type(current[part]) ~= "table" then return nil end
        current = current[part]
    end
    local leaf = parts[#parts]
    if current[leaf] ~= nil and tostring(current[leaf]) ~= tostring(value) then return nil end
    current[leaf] = value
    return true
end

local function apply_bindings(route, request, path_parameters)
    local marked = {}
    local bindings = route.bindings or {}
    for source, destination in pairs(bindings.path or {}) do
        if not set_path(request, destination, path_parameters[source]) then return nil, "CONFLICTING_PATH_BINDING" end
        marked["$." .. destination] = true
    end
    local query, query_error = ngx.req.get_uri_args(100)
    if query_error == "truncated" then return nil, "TOO_MANY_QUERY_ARGUMENTS" end
    local allowed_query = bindings.query or {}
    for name in pairs(query) do
        if not allowed_query[name] then return nil, "UNKNOWN_QUERY_ARGUMENT" end
    end
    for source, destination in pairs(allowed_query) do
        local value = query[source]
        if value ~= nil then
            if not set_path(request, destination, value) then return nil, "CONFLICTING_QUERY_BINDING" end
            marked["$." .. destination] = true
        end
    end
    return marked
end

local function response_cookies(response)
    return response.headers["Set-Cookie"] or response.headers["set-cookie"]
end

local function copy_response_headers(response)
    local cookies = response_cookies(response)
    if cookies then ngx.header["Set-Cookie"] = cookies end
    local request_id = response.headers["X-Request-Id"] or response.headers["x-request-id"]
    if request_id then ngx.header["X-Request-Id"] = request_id end
end

function M.handle()
    local route, path_parameters = find_route(ngx.req.get_method(), ngx.var.uri)
    if not route then return errors.send(404, "ROUTE_NOT_FOUND") end
    local valid, status, policy_error = policy.validate(route)
    if not valid then return errors.send(status, policy_error) end
    if route.local_handler == "csrf" then return csrf.bootstrap(route.csrf_path) end

    local request, body_status, body_error, detail = policy.read_json(route)
    if not request then return errors.send(body_status, body_error, detail) end
    local binding_paths, binding_error = apply_bindings(route, request, path_parameters)
    if not binding_paths then return errors.send(400, binding_error) end
    local encoded, encode_error = codec.encode(route.request_type, request, binding_paths)
    if not encoded then return errors.send(400, "INVALID_JSON_REQUEST", encode_error) end
    local response, upstream_error, upstream_detail = upstream.call(route, encoded)
    if not response then
        local http_status = upstream_error == "UPSTREAM_TIMEOUT" and 504 or 502
        if upstream_detail then ngx.log(ngx.WARN, upstream_error, ": ", upstream_detail) end
        return errors.send(http_status, upstream_error, nil, true)
    end
    copy_response_headers(response)
    local content_type = response.headers["Content-Type"] or response.headers["content-type"] or ""
    content_type = content_type:lower():match("^%s*([^; ]+)") or ""
    if content_type ~= "application/proto" then
        ngx.log(ngx.WARN, "unexpected upstream content type: ", content_type)
        return errors.send(502, "INVALID_UPSTREAM_RESPONSE")
    end
    local decoded, decode_error = codec.decode(route.response_type, response.body)
    if not decoded then
        ngx.log(ngx.WARN, "invalid upstream protobuf: ", tostring(decode_error))
        return errors.send(502, "INVALID_UPSTREAM_RESPONSE")
    end
    ngx.status = errors.http_status(decoded)
    ngx.header["Content-Type"] = "application/json; charset=utf-8"
    ngx.header["Cache-Control"] = "no-store"
    ngx.print(codec.json_encode(decoded))
    return ngx.exit(ngx.status)
end

M.find_route = find_route
M.set_path = set_path
M.copy_response_headers = copy_response_headers
M.response_cookies = response_cookies
return M
