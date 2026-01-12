const parse_path = (path, x = 0, y = 0) => {
    let at = 0;
    path = path.replaceAll(/[A-Za-z]/g, str => ' ' + str + ' ').replaceAll(",", " ").replaceAll("-", " -");
    path = path.slice(1, path.length - 1).split(' ').filter(x => x);
    path = path.map(x => {
        x = x.split(".");
        if (x.length == 1) return x;
        if (x.length == 2) return x[0] + "." + x[1];
        let first = x.shift();
        x = x.map(y => "0." + y);
        x[0] = first + x[0].slice(1);
        return x;
    }).flat();
    let ret_str = 'ctx.begin_path();\n';
    let curr_op = 'm';
    const bx = x, by = y;
    const op_parse = _ => {
        switch(curr_op)
        {
            case 'M':
                x = parseFloat(path[at++]) + bx;
                y = parseFloat(path[at++]) + by;
                ret_str += `rr_renderer_move_to(renderer, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            case 'L':
            {
                const prev_x = x;
                const prev_y = y;
                x = parseFloat(path[at++]) + bx;
                y = parseFloat(path[at++]) + by;
                if (Math.abs(x - prev_x) > 0.001 || Math.abs(y - prev_y) > 0.001)
                    ret_str += `rr_renderer_line_to(renderer, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            }
            case 'H':
            {
                const prev_x = x;
                x = parseFloat(path[at++]) + bx;
                if (Math.abs(x - prev_x) > 0.001)
                    ret_str += `rr_renderer_line_to(renderer, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            }
            case 'V':
            {
                const prev_y = y;
                y = parseFloat(path[at++]) + by;
                if (Math.abs(y - prev_y) > 0.001)
                    ret_str += `rr_renderer_line_to(renderer, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            }
            case 'Q':
            {
                const x1 = parseFloat(path[at++]) + bx;
                const y1 = parseFloat(path[at++]) + by;
                x = parseFloat(path[at++]) + bx;
                y = parseFloat(path[at++]) + by;
                ret_str += `rr_renderer_quadratic_curve_to(renderer, ${x1.toFixed(2)}, ${y1.toFixed(2)}, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            }
            case 'C':
            {
                const x1 = parseFloat(path[at++]) + bx;
                const y1 = parseFloat(path[at++]) + by;
                const x2 = parseFloat(path[at++]) + bx;
                const y2 = parseFloat(path[at++]) + by;
                x = parseFloat(path[at++]) + bx;
                y = parseFloat(path[at++]) + by;
                ret_str += `rr_renderer_bezier_curve_to(renderer, ${x1.toFixed(2)}, ${y1.toFixed(2)}, ${x2.toFixed(2)}, ${y2.toFixed(2)}, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            }
            case 'm':
                x += parseFloat(path[at++]);
                y += parseFloat(path[at++]);
                ret_str += `rr_renderer_move_to(renderer, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            case 'l':
            {
                const prev_x = x;
                const prev_y = y;
                x += parseFloat(path[at++]);
                y += parseFloat(path[at++]);
                if (Math.abs(x - prev_x) > 0.001 || Math.abs(y - prev_y) > 0.001)
                    ret_str += `rr_renderer_line_to(renderer, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            }
            case 'h':
            {
                const prev_x = x;
                x += parseFloat(path[at++]);
                if (Math.abs(x - prev_x) > 0.001)
                    ret_str += `rr_renderer_line_to(renderer, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            }
            case 'v':
            {
                const prev_y = y;
                y += parseFloat(path[at++]);
                if (Math.abs(y - prev_y) > 0.001)
                    ret_str += `rr_renderer_line_to(renderer, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            }
            case 'q':
            {
                const x1 = x + parseFloat(path[at++]);
                const y1 = y + parseFloat(path[at++]);
                x += parseFloat(path[at++]);
                y += parseFloat(path[at++]);
                ret_str += `rr_renderer_quadratic_curve_to(renderer, ${x1.toFixed(2)}, ${y1.toFixed(2)}, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            }
            case 'c':
            {
                const x1 = x + parseFloat(path[at++]);
                const y1 = y + parseFloat(path[at++]);
                const x2 = x + parseFloat(path[at++]);
                const y2 = y + parseFloat(path[at++]);
                x += parseFloat(path[at++]);
                y += parseFloat(path[at++]);
                ret_str += `rr_renderer_bezier_curve_to(renderer, ${x1.toFixed(2)}, ${y1.toFixed(2)}, ${x2.toFixed(2)}, ${y2.toFixed(2)}, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            }
            case 'z':
                ret_str += `rr_renderer_close_path(renderer);\n`;
                break;
            case 's':
                const x1 = x + parseFloat(path[at++]);
                const y1 = y + parseFloat(path[at++]);
                const x2 = x + parseFloat(path[at++]);
                const y2 = y + parseFloat(path[at++]);
                x += parseFloat(path[at++]);
                y += parseFloat(path[at++]);
                ret_str += `rr_renderer_bezier_curve_to(renderer, ${x1.toFixed(2)}, ${y1.toFixed(2)}, ${x2.toFixed(2)}, ${y2.toFixed(2)}, ${x.toFixed(2)}, ${y.toFixed(2)});\n`;
                break;
            default:
                console.log(curr_op);
                throw new Error('uh oh: ' + path[at] + ' ' + path.slice(at - 10, at + 10));
        }
    }
    while (at < path.length)
    {
        if (!Number.isFinite(parseFloat(path[at])))    
            curr_op = path[at++];
        console.log(at, path.length);
        op_parse();
    }
    return ret_str;
}

const parse_svg = str => {
    let doc = new DOMParser().parseFromString(str, 'text/xml');
    const [offset_x, offset_y] = doc.getElementsByTagName('svg')[0].attributes.viewBox.value.split(' ').map(x => parseFloat(x)).slice(2);
    doc = [...doc.getElementsByTagName('path')].map(x => x.attributes).filter(x => !x["clip-rule"] && (!x["fill-opacity"] || parseFloat(x["fill-opacity"].nodeValue) !== 0));
    let ret = "";
    for (let x of doc)
    {
        if (x["fill"])
        {
            if (x["fill-opacity"])
                ret += `rr_renderer_set_fill(renderer, 0x${((x["fill-opacity"].nodeValue * 255) | 0).toString(16).padStart(2, '0')}${x["fill"].nodeValue.slice(1)});\n`;
            else
                ret += `rr_renderer_set_fill(renderer, 0xff${x["fill"].nodeValue.slice(1)});\n`;
        }
        ret += parse_path(x["d"].nodeValue, -offset_x / 2, -offset_y / 2);
        if (x["fill"])
            ret += `rr_renderer_fill(renderer);\n`;
    }
    console.log(ret);
}

const transform_path = str => str.replaceAll(");","").replaceAll("path.MoveTo(","M").replaceAll("path.BezierCurveTo(","C").replaceAll("path.QuadraticCurveTo(","Q").replaceAll("path.LineTo(","L").replaceAll('\n', ' ').replaceAll("path.ClosePath(", "");

console.log(parse_path(`M231.8 122.8c.6 5-2.7 7.6-7.4 5.6s-5.2-6.1-1.2-9.2c4.1-3 8-1.4 8.6 3.6M171 104.9c-1.6 5.3-6 6.3-9.7 2.3L148.8 94c-3.8-4-2.5-8.3 2.9-9.6l17.7-4.2c5.4-1.3 8.4 2 6.9 7.3zm-117.3-52c2 5.1-.8 8.6-6.2 7.8L22.6 57c-5.4-.8-7.1-5-3.6-9.3l15.7-19.6c3.4-4.3 7.9-3.6 9.9 1.5zm165.4-18.2c-2.1 4.4-6 4.8-8.8.8s-1.1-7.6 3.7-8c4.9-.5 7.2 2.7 5.1 7.2m-32 160.5c-3 4.6-7.5 4.3-10-.6l-3.5-7c-2.5-4.9 0-8.7 5.5-8.3l7.8.5c5.5.3 7.5 4.3 4.5 8.9zm-134.9 43c-1.6 5.3-6 6.3-9.7 2.3l-11-11.6c-3.8-4-2.5-8.3 2.9-9.6l15.6-3.7c5.4-1.3 8.4 2 6.9 7.3zM65.3 167c2.9 4.1 1.1 7.8-3.8 8.3s-7.4-2.9-5.2-7.5c2-4.6 6.1-4.9 9-.8m58.2-23c0 5.5-3.9 7.8-8.7 5l-5.1-3c-4.8-2.8-4.8-7.2 0-10l5.1-3c4.8-2.8 8.7-.5 8.7 5z`));