const id = "PM_EVENT_PLATPLUS_STATE_SCRIPT_CLEAR";
const groups = ["Platformer+"];
const name = "Remove a Script from A Platformer+ State";

const fields = [
    {
        key: "state",
        label: "Select Player State",
        type: "select",
        defaultValue: "0",
        options: [
          ["0", "Start Falling"],
          ["1", "Falling"],
          ["2", "End Falling"],
          ["3", "Start Grounded"],
          ["4", "Grounded"],
          ["5", "End Grounded"],
          ["6", "Start Jumping"],
          ["7", "Jumping"],
          ["8", "End Jumping"],
          ["9", "Start Dashing"],
          ["10", "Dashing"],
          ["11", "End Dashing"],
          ["12", "Start Climbing Ladder"],
          ["13", "Climbing Ladder"],
          ["14", "End Climbing Ladder"],
          ["15", "Start Wall Slide"],
          ["16", "Wall Sliding"],
          ["17", "End Wall Slide"],
          ["18", "Knockback State Start"],
          ["19", "Knockback State"],
          ["21", "Blank State Start"],
          ["22", "Blank State"]
        ],
    },
  ];
  
  const compile = (input, helpers) => {
    const {appendRaw, _addComment} = helpers;

    const stateNumber = `${input.state}`;

    _addComment("Remove Platformer State Script");
    appendRaw(`VM_PUSH_CONST ${stateNumber}`);
    appendRaw(`VM_CALL_NATIVE b_clear_state_script, _clear_state_script`);
    appendRaw(`VM_POP 1`);
  };
  
  module.exports = {
    id,
    name,
    groups,
    fields,
    compile,
    allowedBeforeInitFade: true,
  };
  
  