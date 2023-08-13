#include <Client/DOM.h>

#include <emscripten.h>

void rr_dom_create_text_element(char const *name, uint32_t text_limit)
{
    EM_ASM({
        const name = Module.ReadCstr($0);
        const elem = document.createElement('input');
        elem.id = name;
        elem.type = "text";
        elem.style.position = "absolute";
        elem.style["font-family"] = "Ubuntu";
        elem.style.display = 'none';
        elem.style["border-width"] = "0px";
        elem.style.background = "transparent";
        elem.style.border = "none";
        elem.style.outline = "none";
        elem.style["padding-left"] = "2px";
        document.body.appendChild(elem);
    }, name, text_limit);
}

void rr_dom_element_show(char const *name)
{
    EM_ASM({
        const name = Module.ReadCstr($0);
        const elem = document.getElementById(name);
        elem.style.display = 'block';
    }, name);
}

void rr_dom_element_hide(char const *name)
{
    EM_ASM({
        const name = Module.ReadCstr($0);
        const elem = document.getElementById(name);
        elem.style.display = 'none';
    }, name);
}

void rr_dom_element_update_position(char const *name, float x, float y, float w, float h)
{
    EM_ASM({
        const name = Module.ReadCstr($0);
        const elem = document.getElementById(name);
        elem.style.left = ($1 - $3 / 2) / devicePixelRatio + "px";
        elem.style.top = ($2 - $4 / 2) / devicePixelRatio + "px";
        elem.style.width = $3 / devicePixelRatio + "px";
        elem.style.height = $4 / devicePixelRatio + "px";
        elem.style["font-size"] = $4 / devicePixelRatio * 0.8 + "px";
    }, name, x, y, w, h);
}

void rr_dom_retrieve_text(char const *name, char *out, uint32_t max_len)
{
    EM_ASM({
        const name = Module.ReadCstr($0);
        const elem = document.getElementById(name);
        let arr = new TextEncoder().encode(elem.value);
        const len = $2 > arr.length ? arr.length : $2;
        // remember off by one errors
        arr = arr.slice(0,len);
        Module.HEAPU8.set(arr, $1);
        Module.HEAPU8[$1 + len] = 0;
    }, name, out, max_len);
}

void rr_copy_string(char const *str)
{
    EM_ASM({
        let elem = document.createElement("textarea");
        elem.style.position = "fixed";
        elem.style.top = 0;
        elem.style.left = 0;
        elem.style.width = "2em";
        elem.style.height = "2em";
        elem.style.margin = 0;
        elem.style.padding = 0;
        elem.style.border = "none";
        elem.style.outline = "none";
        elem.style.boxShadow = "none";
        elem.style.background = "transparent";
        elem.value = Module.ReadCstr($0);
        document.body.appendChild(elem);
        elem.focus();
        elem.select();
        try {
            document.execCommand("copy") || console.log("execCommand copy failed");
        } catch (e) {}
        document.body.removeChild(elem);
    }, str);
}