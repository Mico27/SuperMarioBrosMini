const id = "PM_EVENT_PLATPLUS_STATE_STORE";
const groups = ["Platformer+", "Player Fields", "EVENT_GROUP_VARIABLES"];
const name = "Store Platformer+ State In a Variable";

const fields = [
  {
    label: "Gets player state and stores it in a variable."
  },
  {
    key: "variable",
    type: "variable",
    defaultValue: "LAST_VARIABLE",
  },
  {
    key: "field",
    defaultValue: "plat_state",
  },
  {    label: "0 = Started Falling, 1 = Falling, 2 = End Fall", },
  {    label: "3 = Started Landing, 4 = On the Ground, 5 = End Ground", },
  {    label: "6 = Started Jumping, 7 = Jumping, 8 = End Jump", },
  {    label: "9 = Started Dashing, 10 = Dashing, 11 = End Dash", },
  {    label: "12 = Started Climbing, 13 = On a Ladder, 14 = End Ladder", },
  {    label: "15 = Started Sliding, 16 = On a Wall, 17 = End Wall", },
  {    label: "18 = Started Knockback, 19 = Knockback", },
  {    label: "20 = Started Blank, 21 = Blank", },
];

const compile = (input, helpers) => {
  const { appendRaw, getVariableAlias, _addComment } = helpers;
  const fieldVarTypeLookup = {
    plat_state: "UINT8",
  };
  const fieldName = `_${input.field}`;
  const variableAlias = getVariableAlias(input.variable);

  _addComment("Store player field in variable");
  appendRaw(
    `VM_GET_${fieldVarTypeLookup[input.field]} ${variableAlias}, ${fieldName}`
  );
};

module.exports = {
  id,
  name,
  groups,
  fields,
  compile,
  allowedBeforeInitFade: true,
};
