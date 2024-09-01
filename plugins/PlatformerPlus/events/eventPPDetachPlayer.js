const id = "PM_EVENT_PLATPLUS_DETACH_PLAYER";
const groups = ["Platformer+", "EVENT_GROUP_VARIABLES"];
const name = "Detach Player from Platform";

const fields = [
    {
      key: "state",
      defaultValue: "0",

    },
    {
      key: "field",
      defaultValue: "actor_attached",
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