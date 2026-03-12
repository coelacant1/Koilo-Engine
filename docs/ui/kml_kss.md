# KML & KSS Reference

**Version:** 1.0
**Date:** 2026-03-12

---

## 1. Overview

KML (Koilo Markup Language) and KSS (Koilo Style Sheet) provide declarative UI for KoiloEngine. KML defines structure with HTML-like tags; KSS styles it with CSS-like rules.

A working example lives in `examples/ui_demo/` with its markup in `assets/ui/editor.kml` and stylesheet in `assets/ui/editor.kss` as well as a screenshot example in `assets/screenshots/ui_demo_1.png`.

### Loading in KoiloScript

```ks
var root = ui.LoadMarkup("assets/ui/editor.kml", "assets/ui/editor.kss");
var btn  = ui.FindWidget("play-btn");

fn OnPlay() { ui.SetText(btn, "Running..."); }
```

**Script API:**

| Method | Description |
|--------|-------------|
| `ui.LoadMarkup(kml, kss)` | Load markup + stylesheet, returns root ID |
| `ui.FindWidget(id)` | Find widget by `id` attribute |
| `ui.SetText(w, s)` / `ui.GetText(w)` | Set/get widget text |
| `ui.SetVisible(w, bool)` | Show/hide widget |
| `ui.SetEnabled(w, bool)` | Enable/disable widget |
| `ui.SetSliderValue(w, n)` | Set slider value |
| `ui.SetChecked(w, bool)` | Set checkbox/radio state |
| `ui.SetDropdownItems(w, items)` | Populate dropdown |

---

## 2. KML - Markup

### Elements

| Tag | Aliases | Purpose |
|-----|---------|---------|
| `panel` | - | Flex container |
| `label` | - | Read-only text |
| `button` | - | Clickable button |
| `textfield` | `input` | Text input |
| `slider` | - | Numeric slider |
| `checkbox` | - | Boolean toggle |
| `dropdown` | `select` | Combo box |
| `radiobutton` | `radio` | Group-based selection |
| `toggleswitch` | `toggle` | On/off switch |
| `numberspinner` | `spinner` | Numeric +/− input |
| `progressbar` | `progress` | Progress bar |
| `colorfield` | - | Color picker |
| `scrollview` | - | Scrollable container |
| `treenode` | - | Collapsible tree node |
| `tabbar` | `tabs` | Tab interface |
| `splitpane` | `split` | Resizable split |
| `dock` | `dockcontainer` | Docking container |
| `floatingpanel` | `float` | Floating window |
| `popupmenu` | `popup` | Context menu |
| `menuitem` | - | Menu entry |
| `separator` | `hr` | Divider line |
| `image` | `img` | Image display |
| `viewport` | - | Render target |

### Attributes

**Common (all elements):**

| Attribute | Description |
|-----------|-------------|
| `id` | Unique identifier |
| `class` | Space-separated style classes |
| `style` | Inline KSS properties |
| `text` | Text content |
| `visible` | `"true"` / `"false"` |
| `enabled` | `"true"` / `"false"` |
| `onclick` | KoiloScript function name |
| `onchange` | KoiloScript function name |
| `placeholder` | Placeholder text |
| `title` | Tooltip text |

**Widget-specific:**

| Widget | Attribute | Description |
|--------|-----------|-------------|
| `slider` | `min`, `max`, `value` | Range and initial value |
| `numberspinner` | `min`, `max`, `value`, `step` | Range, value, step |
| `checkbox` | `checked`, `group` | State and radio group |
| `progressbar` | `value` | Progress (0–100) |
| `treenode` | `expanded`, `depth`, `has-children` | Tree state |
| `floatingpanel` | `title` | Window title bar |
| `splitpane` | `horizontal` / `vertical` | Split direction |

### Example

```xml
<dock id="main">
  <panel class="toolbar">
    <button onclick="OnPlay">▶ Play</button>
    <separator />
    <label id="status">Ready</label>
  </panel>

  <splitpane horizontal="true">
    <panel class="sidebar">
      <label class="heading">Hierarchy</label>
      <scrollview>
        <treenode text="Scene" expanded="true">
          <treenode text="Camera" depth="1" />
          <treenode text="Player" depth="1" />
        </treenode>
      </scrollview>
    </panel>

    <viewport id="game-view" />
  </splitpane>
</dock>
```

---

## 3. KSS - Styling

KSS follows CSS syntax. Separate structure from style by placing rules in `.kss` files.

### Selectors

| Selector | Example | Specificity |
|----------|---------|-------------|
| Element | `button { }` | 1 |
| Class | `.primary { }` | 10 |
| ID | `#play-btn { }` | 100 |
| Universal | `* { }` | 0 |
| Child | `panel > label { }` | - |
| Descendant | `panel label { }` | - |
| Adjacent sibling | `button + label { }` | - |
| General sibling | `button ~ label { }` | - |
| Attribute | `[disabled] { }` | 10 |
| Compound | `button.primary { }` | 11 |

**Pseudo-classes:** `:hover`, `:active` / `:pressed`, `:focus`, `:focus-visible`, `:selected`, `:disabled`, `:checked`

**Media queries:**
```css
@media (max-width: 1024px) {
    .sidebar { width: 180px; }
}
```

### CSS Variables

```css
:root {
    --bg: #1e1e24;
    --accent: #4a7dc8;
}

button { background-color: var(--bg); }
button:hover { color: var(--accent); }
```

### Properties

#### Layout

| Property | Values |
|----------|--------|
| `display` | `row`, `column` |
| `width`, `height` | `100px`, `50%`, `fill`, `fit-content`, `auto` |
| `min-width`, `max-width` | size value |
| `min-height`, `max-height` | size value |
| `padding` | `10px`, `10px 20px`, 4-value shorthand |
| `margin` | `10px`, `auto`, 4-value shorthand |
| `gap` | `10px` |
| `flex-grow`, `flex-shrink` | number |
| `flex-basis` | size value |
| `flex` | shorthand (`1`, `0 0 100px`) |
| `flex-wrap` | `wrap`, `nowrap` |
| `align-items` | `start`, `center`, `end`, `stretch`, `space-between`, `space-around` |
| `justify-content` | `start`, `center`, `end`, `space-between`, `space-around`, `space-evenly` |
| `align-self` | `auto`, `start`, `center`, `end`, `stretch` |
| `position` | `static`, `relative`, `absolute` |
| `top`, `right`, `bottom`, `left` | offset value |
| `overflow` | `visible`, `hidden`, `scroll` |
| `z-index` | integer |
| `aspect-ratio` | `16/9`, `1` |
| `box-sizing` | `content-box`, `border-box` |

#### Grid

| Property | Values |
|----------|--------|
| `grid-template-columns` | `1fr 2fr`, `100px auto` |
| `grid-template-rows` | `auto 100px` |
| `grid-column`, `grid-row` | `1 / 3` |
| `column-gap`, `row-gap` | size value |

#### Typography

| Property | Values |
|----------|--------|
| `font-size` | `14px`, `1.5em` |
| `font-weight` | `normal`, `bold`, `400`–`700` |
| `color` | color value |
| `text-align` | `left`, `center`, `right` |
| `line-height` | `1.5`, `20px` |
| `letter-spacing` | `1px` |
| `text-decoration` | `none`, `underline`, `line-through`, `overline` |
| `text-overflow` | `visible`, `hidden`, `ellipsis` |
| `white-space` | `normal`, `nowrap`, `pre` |

#### Visual

| Property | Values |
|----------|--------|
| `background-color` | color value |
| `background` | color or `linear-gradient(to bottom, #c1, #c2)` |
| `opacity` | `0.0`–`1.0` |
| `border` | `1px solid #333` |
| `border-width`, `border-color` | individual values |
| `border-radius` | `5px`, 4-value shorthand |
| `box-shadow` | `0px 2px 8px rgba(0,0,0,0.5)` |
| `accent-color` | color (checkbox, slider, radio) |
| `placeholder-color` | color |
| `caret-color` | color |
| `cursor` | `default`, `pointer`, `text`, `grab`, `move`, `not-allowed`, etc. |
| `visibility` | `visible`, `hidden`, `collapse` |
| `pointer-events` | `auto`, `none` |

#### Transitions

| Property | Values |
|----------|--------|
| `transition` | `0.15s`, `background-color 0.2s` |
| `transform` | `rotate(45deg)`, `scale(1.5)`, `translate(10px, 20px)` |

### Units

| Unit | Meaning |
|------|---------|
| `px` | Pixels |
| `%` | Percentage of parent |
| `em` / `rem` | Relative to font size / root font size |
| `vw` / `vh` | 1% of viewport width / height |
| `fill` | Fill remaining space |
| `fit-content` | Size to content |
| `auto` | Automatic |
| bare number | Treated as px |

### Colors

`#RGB`, `#RRGGBB`, `#RRGGBBAA`, `rgb(r,g,b)`, `rgba(r,g,b,a)`

### Example

```css
:root {
    --bg-dark: #16161a;
    --text: #e0e0e8;
    --accent: #4a7dc8;
    --border: #333340;
    --radius: 5px;
}

panel {
    background-color: var(--bg-dark);
    display: column;
    padding: 4px;
}

button {
    background-color: #2a2a36;
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 6px 12px;
    cursor: pointer;
    transition: 0.15s;
}

button:hover {
    background-color: #383848;
    color: var(--accent);
}

.toolbar {
    display: row;
    height: 36px;
    gap: 4px;
    align-items: center;
    padding: 0px 8px;
    border-bottom: 1px solid var(--border);
}

.sidebar {
    width: 240px;
    display: column;
    overflow: scroll;
}

@media (max-width: 800px) {
    .sidebar { width: 160px; }
}
```

---

## 4. Key Differences from HTML/CSS

| | KML/KSS | HTML/CSS |
|-|-----------|----------|
| Tags | Widget-specific (`slider`, `treenode`) | Semantic (`div`, `input`) |
| Layout | `display: row` / `column` | `display: flex` with `flex-direction` |
| Events | `onclick="FnName"` (no parens) | `onclick="fn()"` (JavaScript) |
| Size | `fill`, `fit-content` keywords | `flex: 1`, `width: fit-content` |
| Rendering | GPU-accelerated engine canvas | Browser DOM |
| Scripting | KoiloScript callbacks | JavaScript DOM API |
