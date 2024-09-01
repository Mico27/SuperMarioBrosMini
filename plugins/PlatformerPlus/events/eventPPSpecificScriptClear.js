const id = "PM_EVENT_PLATMARIO_SPECIFIC_SCRIPT_CLEAR";
const groups = ["Mario Platformer"];
const name = "Remove a Script from A Mario Platformer Specific Event";

const fields = [
    {
        key: "specific_event",
        label: "Select Specific Event",
        type: "select",
        defaultValue: "0",
        options: [
          ["0", "Coins collected"],
          ["1", "Hit block"],
        ],
    },
  ];
  
  const compile = (input, helpers) => {
    const {appendRaw, _addComment} = helpers;

    const specific_event = `${input.specific_event}`;

    _addComment("Remove Mario Platformer Specific Event Script");
    appendRaw(`VM_PUSH_CONST ${specific_event}`);
    appendRaw(`VM_CALL_NATIVE b_clear_specific_script, _clear_specific_script`);
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
  
  