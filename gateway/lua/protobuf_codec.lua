local pb = require "pb"
local cjson = require "cjson.safe"
pb.option("int64_as_string")

local M = {}
local cache = {}

local integer_limits = {
    int32 = {"-2147483648", "2147483647"},
    sint32 = {"-2147483648", "2147483647"},
    sfixed32 = {"-2147483648", "2147483647"},
    uint32 = {"0", "4294967295"},
    fixed32 = {"0", "4294967295"},
    int64 = {"-9223372036854775808", "9223372036854775807"},
    sint64 = {"-9223372036854775808", "9223372036854775807"},
    sfixed64 = {"-9223372036854775808", "9223372036854775807"},
    uint64 = {"0", "18446744073709551615"},
    fixed64 = {"0", "18446744073709551615"},
}

local int64_types = { int64=true, sint64=true, sfixed64=true, uint64=true, fixed64=true }
local numeric_types = {
    int32=true, sint32=true, sfixed32=true, uint32=true, fixed32=true,
    int64=true, sint64=true, sfixed64=true, uint64=true, fixed64=true,
    float=true, double=true,
}

local function normalize_type(type_name)
    return type_name and type_name:gsub("^%.", "")
end

local function descriptor_kind(type_name)
    local _, _, kind = pb.type(normalize_type(type_name))
    return kind
end

local function schema(type_name)
    type_name = normalize_type(type_name)
    if cache[type_name] then return cache[type_name] end
    local result = {}
    cache[type_name] = result
    for name, number, field_type, default, label in pb.fields(type_name) do
        result[name] = {
            number = number,
            type = normalize_type(field_type),
            default = default,
            repeated = label == "repeated",
            kind = descriptor_kind(field_type),
        }
    end
    return result
end

local function unsigned_compare(left, right)
    left = left:gsub("^0+", "")
    right = right:gsub("^0+", "")
    if left == "" then left = "0" end
    if right == "" then right = "0" end
    if #left ~= #right then return #left < #right and -1 or 1 end
    if left == right then return 0 end
    return left < right and -1 or 1
end

local function decimal_in_range(value, minimum, maximum)
    if not value:match("^-?%d+$") then return false end
    local negative = value:sub(1, 1) == "-"
    local min_negative = minimum:sub(1, 1) == "-"
    if negative then
        if not min_negative then return false end
        local magnitude = value:sub(2)
        return unsigned_compare(magnitude, minimum:sub(2)) <= 0
    end
    return unsigned_compare(value, maximum) <= 0
end

local function array(value)
    if type(value) ~= "table" then return false end
    local count = 0
    for key in pairs(value) do
        if type(key) ~= "number" or key < 1 or key % 1 ~= 0 then return false end
        count = math.max(count, key)
    end
    return count == #value
end

local validate_message

local function validate_scalar(field, value, path, from_binding, binding_paths)
    local field_type = field.type
    if field.kind == "message" then
        if type(value) ~= "table" then return nil, path .. " must be an object" end
        return validate_message(field_type, value, path, binding_paths)
    end
    if field.kind == "enum" then
        if type(value) ~= "string" or pb.enum(field_type, value) == nil then
            return nil, path .. " must be a valid enum name"
        end
        return value
    end
    if field_type == "string" then
        if type(value) ~= "string" then return nil, path .. " must be a string" end
        return value
    end
    if field_type == "bytes" then
        if type(value) ~= "string" then return nil, path .. " must be Base64" end
        local decoded = ngx.decode_base64(value)
        if not decoded or ngx.encode_base64(decoded) ~= value then return nil, path .. " contains invalid Base64" end
        return decoded
    end
    if field_type == "bool" then
        if from_binding and type(value) == "string" then
            if value == "true" then return true end
            if value == "false" then return false end
        end
        if type(value) ~= "boolean" then return nil, path .. " must be a boolean" end
        return value
    end
    if numeric_types[field_type] then
        if int64_types[field_type] then
            if type(value) ~= "string" then return nil, path .. " must be a decimal string" end
            local limits = integer_limits[field_type]
            if not decimal_in_range(value, limits[1], limits[2]) then return nil, path .. " is out of range" end
            if field_type == "uint64" or field_type == "fixed64" then return "#" .. value end
            return value
        end
        if from_binding and type(value) == "string" and value:match("^-?%d+$") then value = tonumber(value) end
        if type(value) ~= "number" or value ~= value or value == math.huge or value == -math.huge then
            return nil, path .. " must be a number"
        end
        if field_type ~= "float" and field_type ~= "double" then
            if value % 1 ~= 0 then return nil, path .. " must be an integer" end
            local limits = integer_limits[field_type]
            if value < tonumber(limits[1]) or value > tonumber(limits[2]) then return nil, path .. " is out of range" end
        end
        return value
    end
    return nil, path .. " has unsupported protobuf type " .. tostring(field_type)
end

validate_message = function(type_name, value, path, binding_paths)
    if type(value) ~= "table" then return nil, path .. " must be an object" end
    local fields = schema(type_name)
    local output = {}
    for name, item in pairs(value) do
        local field = fields[name]
        if not field then return nil, path .. "." .. tostring(name) .. " is unknown" end
        local field_path = path .. "." .. name
        local from_binding = binding_paths and binding_paths[field_path]
        if field.repeated then
            local values = item
            if from_binding and type(values) == "string" then
                values = {}
                for part in item:gmatch("[^,]+") do values[#values + 1] = part end
            end
            if not array(values) then return nil, field_path .. " must be an array" end
            output[name] = {}
            for index, element in ipairs(values) do
                local checked, check_error = validate_scalar(field, element,
                    field_path .. "[" .. index .. "]", from_binding, binding_paths)
                if checked == nil then return nil, check_error end
                output[name][index] = checked
            end
        else
            local checked, check_error = validate_scalar(field, item, field_path, from_binding, binding_paths)
            if checked == nil then return nil, check_error end
            output[name] = checked
        end
    end
    return output
end

local function output_scalar(field, value)
    if field.kind == "message" then return M.to_json_table(field.type, value) end
    if field.type == "bytes" then return ngx.encode_base64(value) end
    if int64_types[field.type] then return tostring(value):gsub("^#", "") end
    if field.kind == "enum" and type(value) == "number" then return pb.enum(field.type, value) end
    return value
end

function M.validate(type_name, value, binding_paths)
    return validate_message(type_name, value, "$", binding_paths)
end

function M.encode(type_name, value, binding_paths)
    local validated, validation_error = M.validate(type_name, value, binding_paths)
    if not validated then return nil, validation_error end
    local ok, encoded = pcall(pb.encode, normalize_type(type_name), validated)
    if not ok then return nil, encoded end
    return encoded
end

function M.decode(type_name, bytes)
    local ok, decoded = pcall(pb.decode, normalize_type(type_name), bytes)
    if not ok or type(decoded) ~= "table" then return nil, decoded end
    return M.to_json_table(type_name, decoded)
end

function M.to_json_table(type_name, value)
    local fields = schema(type_name)
    local output = {}
    for name, item in pairs(value) do
        local field = fields[name]
        if not field then return nil end
        if field.repeated then
            if #item == 0 then
                output[name] = cjson.empty_array
            else
                output[name] = {}
                for index, element in ipairs(item) do output[name][index] = output_scalar(field, element) end
            end
        else
            output[name] = output_scalar(field, item)
        end
    end
    return output
end

function M.json_encode(value)
    return cjson.encode(value)
end

M.schema = schema
return M
