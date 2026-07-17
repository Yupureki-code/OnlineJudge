local error_response = require "error_response"

local M = {}

local function cookie(name)
    local header = ngx.var.http_cookie or ""
    for key, value in header:gmatch("%s*([^=;]+)=([^;]*)") do
        if key == name then return value end
    end
end

local function constant_time_equal(left, right)
    if type(left) ~= "string" or type(right) ~= "string" then return false end
    local bit = require "bit"
    local difference = bit.bxor(#left, #right)
    local length = math.max(#left, #right)
    for index = 1, length do
        difference = bit.bor(difference,
            bit.bxor(left:byte(index) or 0, right:byte(index) or 0))
    end
    return difference == 0
end

local function token()
    local random = require "resty.random"
    local bytes = random.bytes(32, true)
    if not bytes then return nil end
    return (ngx.encode_base64(bytes):gsub("+", "-"):gsub("/", "_"):gsub("=+$", ""))
end

function M.bootstrap(path)
    local value = token()
    if not value then return error_response.send(500, "CSRF_GENERATION_FAILED") end
    local secure = (os.getenv("OJ_COOKIE_SECURE") or "true") == "true"
    local attributes = "; Path=" .. (path or "/") .. "; Max-Age=1800; SameSite=Lax"
    if secure then attributes = attributes .. "; Secure" end
    ngx.header["Set-Cookie"] = "oj_csrf_token=" .. value .. attributes
    ngx.header["Content-Type"] = "application/json; charset=utf-8"
    ngx.header["Cache-Control"] = "no-store"
    ngx.say('{"csrf_token":"' .. value .. '"}')
    return ngx.exit(200)
end

function M.verify()
    if not constant_time_equal(cookie("oj_csrf_token"), ngx.var.http_x_csrf_token) then
        return nil, "CSRF_TOKEN_INVALID"
    end
    return true
end

M.constant_time_equal = constant_time_equal
return M
