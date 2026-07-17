local pb = require "pb"
local routes = require "routes"

local M = {}
local loaded = false

local function require_message(type_name)
    if not pb.type(type_name) then
        error("missing protobuf message type: " .. type_name)
    end
end

function M.init()
    if loaded then
        error("protobuf descriptor loaded more than once in worker")
    end
    local descriptor = os.getenv("OJ_PROTO_DESCRIPTOR") or "/usr/local/openresty/oj/onlinejudge.pb"
    local ok, offset = pb.loadfile(descriptor)
    if not ok then
        error("failed to load protobuf descriptor at offset " .. tostring(offset))
    end
    for _, route in ipairs(routes) do
        if not route.local_handler then
            require_message(route.request_type)
            require_message(route.response_type)
        end
    end
    loaded = true
end

function M.loaded()
    return loaded
end

return M
