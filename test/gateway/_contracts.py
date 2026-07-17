import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def _varint(data, offset):
    value = 0
    shift = 0
    while True:
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        if byte < 0x80:
            return value, offset
        shift += 7


def fields(data):
    offset = 0
    while offset < len(data):
        key, offset = _varint(data, offset)
        number, wire = key >> 3, key & 7
        if wire == 0:
            value, offset = _varint(data, offset)
        elif wire == 2:
            length, offset = _varint(data, offset)
            value = data[offset:offset + length]
            offset += length
        elif wire == 1:
            value, offset = data[offset:offset + 8], offset + 8
        elif wire == 5:
            value, offset = data[offset:offset + 4], offset + 4
        else:
            raise ValueError(f"unsupported protobuf wire type {wire}")
        yield number, wire, value


def _text(data):
    return data.decode("utf-8")


def parse_descriptor(path=None):
    path = path or ROOT / "gateway/generated/onlinejudge.pb"
    result = {"messages": set(), "services": {}}
    for number, _, file_data in fields(Path(path).read_bytes()):
        if number != 1:
            continue
        package = ""
        messages = []
        services = []
        for field, _, value in fields(file_data):
            if field == 2:
                package = _text(value)
            elif field == 4:
                messages.append(value)
            elif field == 6:
                services.append(value)
        for message in messages:
            name = next(_text(v) for n, _, v in fields(message) if n == 1)
            result["messages"].add(f"{package}.{name}")
        for service in services:
            service_name = None
            methods = {}
            for field, _, value in fields(service):
                if field == 1:
                    service_name = _text(value)
                elif field == 2:
                    method = {n: _text(v).lstrip(".") for n, _, v in fields(value) if n in (1, 2, 3)}
                    methods[method[1]] = (method[2], method[3])
            result["services"][f"{package}.{service_name}"] = methods
    return result


ROUTE_RE = re.compile(
    r'(?P<area>main|admin)\("(?P<verb>[A-Z]+)",\s*"(?P<path>[^"]+)",\s*'
    r'"(?P<method>[^"]+)",\s*"(?P<request>[^"]+)",\s*"(?P<response>[^"]+)",\s*'
    r'"(?P<auth>[^"]+)",\s*(?P<csrf>true|false)', re.MULTILINE)
DENIED_RE = re.compile(r'\["(?P<method>oj\.biz\.[^"]+)"\]\s*=\s*"(?P<reason>[^"]+)"')


def parse_routes():
    source = (ROOT / "gateway/routes.lua").read_text()
    routes = [match.groupdict() for match in ROUTE_RE.finditer(source)]
    denied = {match.group("method"): match.group("reason") for match in DENIED_RE.finditer(source)}
    return routes, denied
