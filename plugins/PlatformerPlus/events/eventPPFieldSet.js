const id = "PM_EVENT_PLATPLUS_FIELD_SET";
const groups = ["Platformer+", "Player Fields", "EVENT_GROUP_VARIABLES"];
const name = "Update Platformer+ Field";

const fields = [
  {
    label: "Note: Changes implemented with this event will not carry over between scenes (use Update Engine Field for that).",
  },  
  {
    key: "field",
    label: "Field",
    type: "select",
    defaultValue: "dj_val",
    options: [
      ["dj_val", "Number of double jumps left"],
      ["wj_val", "Number of wall jumps left"],
      ["nocollide", "Frames of drop-through."],
      ["jump_per_frame", "Jump amount (per frame)"],
      ["plat_hold_jump_max", "Jump Frames"],
      ["boost_val", "Jump increase from running (per frame)"],
      ["dash_dist", "Dash distance (per frame)"],
    ],
  },
  {
    type: "group",
    width: "50%",
    fields: [
      {
        key: "variable",
        type: "variable",
        defaultValue: "LAST_VARIABLE",
        conditions: [
          {
            key: "type",
            eq: "variable",
          },
        ],
      },
      {
        key: "value",
        type: "number",
        defaultValue: 0,
        min: 0,
        max: 16384,
        conditions: [
          {
            key: "type",
            eq: "number",
          },
        ],
      },
      {
        key: "type",
        type: "selectbutton",
        options: [
          ["variable", "variable"],
          ["number", "number"],
        ],
        inline: true,
        defaultValue: "number",
      },
    ],
  },
  {
    label: "Some variables are listed as per frame. The event does not have access to how many frames the jump and dash are set to, so you must do the division yourself. Additionally, raising the jump amounts too high has the potential to break your jump (try lower values if it stops working).",
  },
];

const compile = (input, helpers) => {
  const { _addComment, _addNL, _setConstMemInt16, _setMemInt16ToVariable } =
    helpers;
  if (input.type === "variable") {
    _addComment("Platformer Plus Field Set To Variable");
    _setMemInt16ToVariable(input.field, input.variable);
  } else {
    _addComment("Platformer Plus Field Set To Value");
    _setConstMemInt16(input.field, input.value);
  }
  _addNL();
};

module.exports = {
  id,
  name,
  groups,
  fields,
  compile,
  allowedBeforeInitFade: true,
};