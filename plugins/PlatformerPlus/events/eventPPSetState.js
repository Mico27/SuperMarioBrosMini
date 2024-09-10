const id = "PM_EVENT_SET_PP_STATE";
const groups = ["Platformer+", "Player Fields"];
const name = "Set Platformer+ State";

const fields = [
  {
	key: "state",
	label: "Select Player State to Set",
	type: "value",
	defaultValue: {
		type: "number",
		value: 0,
	},
  }
];

const compile = (input, helpers) => {
  const { _addComment, _addNL, _declareLocal, variableSetToScriptValue, _setMemInt16ToVariable } =
    helpers;
	
	const tmp_0 = _declareLocal("tmp_0", 1, true);
	variableSetToScriptValue(tmp_0, input.state);
    _addComment("Set Platformer Plus State");
    _setMemInt16ToVariable("que_state", tmp_0);

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
