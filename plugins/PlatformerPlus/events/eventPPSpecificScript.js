const id = "PM_EVENT_PLATMARIO_SPECIFIC_SCRIPT";
const groups = ["Mario Platformer"];
const name = "Attach a Script to A Mario Platformer Specific Event";

const fields = [
    {
        key: "specific_event",
        label: "Select Specific Event",
        type: "select",
        defaultValue: "0",
        options: [
          ["0", "Coins collected"],
          ["1", "Hit block"],
		  ["2", "Enter down pipe"],
		  ["3", "Enter right pipe"],
		  ["4", "Beanstalk transition"],
		  ["5", "Fell in pit"],
        ],
    },
    {
        key: "__scriptTabs",
        type: "tabs",
        defaultValue: "scriptinput",
        values: {
          scriptinput: "On Specific Event",
        },
    },
    {
        key: "script",
        label: "Specific Event Script",
        description: "Specific Event Script",
        type: "events",
        allowedContexts: ["global", "entity"],
        conditions: [
          {
            key: "__scriptTabs",
            in: [undefined, "scriptinput"],
          },
        ],
      },
  ];
  
  const compile = (input, helpers) => {
    const {appendRaw, _compileSubScript, _addComment, vm_call_native, event } = helpers;
    const ScriptRef = _compileSubScript("specific_event", input.script, "test_symbol"+input.specific_event);
    const stateNumber = `${input.specific_event}`;
    const bank = `___bank_${ScriptRef}`;
    const ptr = `_${ScriptRef}`

    _addComment("Set Mario Platformer Specific Event Script");
    appendRaw(`VM_PUSH_CONST ${stateNumber}`);
    appendRaw(`VM_PUSH_CONST ${bank}`);
    appendRaw(`VM_PUSH_CONST ${ptr}`);
    appendRaw(`VM_CALL_NATIVE b_assign_specific_script, _assign_specific_script`);
    appendRaw(`VM_POP 3`);
  };
  
  module.exports = {
    id,
    name,
    groups,
    fields,
    compile,
    allowedBeforeInitFade: true,
  };
  
  