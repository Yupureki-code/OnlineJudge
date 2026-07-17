(function (global) {
    "use strict";

    var sanitizeOptions = {
        USE_PROFILES: { html: true },
        FORBID_TAGS: ["form", "input", "button", "textarea", "select", "option", "iframe", "object", "embed", "style", "template"],
        FORBID_ATTR: ["style"]
    };

    function render(source) {
        if (!global.marked || !global.DOMPurify) return null;
        return global.DOMPurify.sanitize(global.marked.parse(String(source || "")), sanitizeOptions);
    }

    global.OJMarkdown = { render: render };
})(window);
