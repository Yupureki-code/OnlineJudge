local cjson = require "cjson.safe"

local M = {}

local status_map = {
    [200] = 200, [201] = 201, [204] = 204,
    [400] = 400, [401] = 401, [403] = 403, [404] = 404, [405] = 405,
    [409] = 409, [410] = 410, [413] = 413, [415] = 415, [422] = 422,
    [429] = 429, [499] = 499, [500] = 500, [501] = 501, [502] = 502,
    [503] = 503, [504] = 504,
}

function M.http_status(response)
    local status = response and response.status
    local code = status and tonumber(status.code)
    if status and status.success == true then return status_map[code] or 200 end
    return status_map[code] or 500
end

function M.send(http_status, code, message, retryable)
    ngx.status = http_status
    ngx.header["Content-Type"] = "application/json; charset=utf-8"
    ngx.header["Cache-Control"] = "no-store"
    local body = {
        status = {
            success = false,
            code = http_status,
            message = code,
            retryable = retryable == true,
        },
    }
    if message and message ~= "" then body.error_detail = message end
    ngx.say(cjson.encode(body))
    return ngx.exit(http_status)
end

return M
