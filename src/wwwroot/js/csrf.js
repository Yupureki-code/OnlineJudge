(function (global) {
    "use strict";

    var nativeFetch = global.fetch.bind(global);
    var tokens = { main: "", admin: "" };

    function areaForPath(path) {
        return path.indexOf("/admin/") === 0 ? "admin" : "main";
    }

    async function getToken(area, forceRefresh) {
        area = area === "admin" ? "admin" : "main";
        if (!forceRefresh && tokens[area]) return tokens[area];
        var path = area === "admin" ? "/admin/api/csrf" : "/api/csrf";
        var response = await nativeFetch(path, {
            method: "GET",
            credentials: "include",
            headers: { "Accept": "application/json" }
        });
        var payload = await response.json();
        if (!response.ok || !payload.csrf_token) {
            throw new Error("CSRF_BOOTSTRAP_FAILED");
        }
        tokens[area] = payload.csrf_token;
        return tokens[area];
    }

    function clear(area) {
        if (area) tokens[areaForPath(area)] = "";
        else tokens = { main: "", admin: "" };
    }

    global.OJCsrf = {
        areaForPath: areaForPath,
        clear: clear,
        getToken: getToken,
        nativeFetch: nativeFetch
    };
})(window);
