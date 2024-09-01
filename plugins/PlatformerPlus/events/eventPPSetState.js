const id = "PM_EVENT_SET_PP_STATE";
const groups = ["Platformer+", "Player Fields"];
const name = "Set Platformer+ State";

const fields = [
  {
    key: "state",
    label: "Select Player State to Set (this feature is still experimental)",
    type: "select",
    defaultValue: "0",
    options: [
      ["0", "Falling"],
      ["3", "Grounded"],
      ["6", "Jumping"],
      ["9", "Dashing"],
      ["12", "On a Ladder"],
      ["15", "On a Wall"],
      ["18", "Knockback"],
      ["20", "Blank"],
    ],
  },
  {
    key: "field",
    defaultValue: "que_state",
  },
];

const compile = (input, helpers) => {
  const { _addComment, _addNL, _setConstMemInt16, _setMemInt16ToVariable } =
    helpers;
    _addComment("Set Platformer Plus State");
    _setConstMemInt16(input.field, input.state);

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
