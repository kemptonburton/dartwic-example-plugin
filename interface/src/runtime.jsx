import {registerPlugin} from "../sdk/runtime.ts";
import plugin from "./plugin.jsx";
import pluginCss from "dartwic-plugin-css";

const styleId = "dartwic-plugin-style-example_device_plugin";
if (typeof document !== "undefined") {
    let style = document.getElementById(styleId);
    if (!style) {
        style = document.createElement("style");
        style.id = styleId;
        document.head.appendChild(style);
    }
    style.textContent = pluginCss;
}

registerPlugin(plugin);
