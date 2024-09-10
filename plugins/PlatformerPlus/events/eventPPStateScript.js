const id = "PM_EVENT_PLATPLUS_STATE_SCRIPT";
const groups = ["Platformer+"];
const name = "Attach a Script to A Platformer+ State";

const fields = [
    {
		key: "state",
		label: "Select Player State",
		type: "number",
		defaultValue: 0,
	},
    {
        key: "__scriptTabs",
        type: "tabs",
        defaultValue: "scriptinput",
        values: {
          scriptinput: "On State",
        },
    },
    {
        key: "script",
        label: "State Script",
        description: "State Script",
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
    const ScriptRef = _compileSubScript("state", input.script, "test_symbol"+input.state);
    const stateNumber = `${input.state}`;
    const bank = `___bank_${ScriptRef}`;
    const ptr = `_${ScriptRef}`

    _addComment("Set Platformer Script");
    appendRaw(`VM_PUSH_CONST ${stateNumber}`);
    appendRaw(`VM_PUSH_CONST ${bank}`);
    appendRaw(`VM_PUSH_CONST ${ptr}`);
    appendRaw(`VM_CALL_NATIVE b_assign_state_script, _assign_state_script`);
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
  
  