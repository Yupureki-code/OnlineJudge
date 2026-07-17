package.path = "/usr/local/openresty/oj/?.lua;" .. package.path
local pb = require "pb"
assert(pb.loadfile("/usr/local/openresty/oj/onlinejudge.pb"))
local codec = require "protobuf_codec"

local encoded, err = codec.encode("oj.biz.GetSubmissionRequest", { submission_id = "18446744073709551615" })
assert(encoded and not err)
assert(not codec.encode("oj.biz.GetSubmissionRequest", { submission_id = 9007199254740992 }))
assert(not codec.encode("oj.biz.GetSubmissionRequest", { submission_id = "18446744073709551616" }))
assert(not codec.encode("oj.biz.GetSubmissionRequest", { submission_id = "1", unknown = true }))

assert(codec.encode("oj.biz.UpdateAvatarRequest", { content = "YQ==", content_type = "image/png" }))
assert(not codec.encode("oj.biz.UpdateAvatarRequest", { content = "not-base64", content_type = "image/png" }))
assert(codec.encode("oj.biz.ListSubmissionsRequest", { submission_status = "SUBMISSION_STATUS_ACCEPTED" }))
assert(not codec.encode("oj.biz.ListSubmissionsRequest", { submission_status = "ACCEPTED" }))
assert(codec.json_encode({ values = require("cjson.safe").empty_array }) == '{"values":[]}')
local list_request = assert(codec.encode("oj.biz.GetQuestionListRequest",
    { page = { page = "1", page_size = "5" } },
    { ["$.page.page"] = true, ["$.page.page_size"] = true }))
local decoded_list_request = assert(pb.decode("oj.biz.GetQuestionListRequest", list_request))
assert(decoded_list_request.page.page == 1 and decoded_list_request.page.page_size == 5)
assert(not codec.decode("oj.biz.GetQuestionListResponse", string.char(0x0a, 0x05, 0x01)))

local routes = require "routes"
local function assert_binding(type_name, path)
    local current = type_name
    local parts = {}
    for part in path:gmatch("[^.]+") do parts[#parts + 1] = part end
    for index, part in ipairs(parts) do
        local field = codec.schema(current)[part]
        assert(field, type_name .. " has no binding field " .. path)
        if index < #parts then
            assert(field.kind == "message", path .. " traverses a scalar")
            current = field.type
        end
    end
end
for _, route in ipairs(routes) do
    if route.request_type then
        for _, group in pairs(route.bindings or {}) do
            if type(group) == "table" then
                for _, destination in pairs(group) do assert_binding(route.request_type, destination) end
            end
        end
    end
end

package.loaded["resty.http"] = {
    new = function()
        return {}
    end,
}
package.loaded["brpc_upstream"] = nil
local upstream = require "brpc_upstream"
assert(upstream.classify_error("read timeout") == "UPSTREAM_TIMEOUT")
assert(upstream.classify_error("connection refused") == "UPSTREAM_UNAVAILABLE")

package.loaded["router"] = nil
local router = require "router"
local cookies = router.response_cookies({ headers = { ["Set-Cookie"] = {"first=1", "second=2"} } })
assert(type(cookies) == "table" and #cookies == 2)

print("codec_test: PASS")
