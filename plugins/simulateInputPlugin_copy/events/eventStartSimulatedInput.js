const KEY_BITS = {
  left: 0x02,
  right: 0x01,
  up: 0x04,
  down: 0x08,
  a: 0x10,
  b: 0x20,
  select: 0x40,
  start: 0x80,
};

const inputDec = (input) => {
  let output = 0;
  if (Array.isArray(input)) {
    for (let i = 0; i < input.length; i++) {
      output |= KEY_BITS[input[i]];
    }
  } else {
    output = KEY_BITS[input];
  }
  return output;
};

export const id = "EVENT_START_SIMULATED_INPUTS";
export const name = "Start simulated inputs";
export const groups = ["EVENT_GROUP_INPUT"];

export const autoLabel = (fetchArg) => {
  return `Start simulated inputs`;
};

export const fields = [  
  {
    key: "input_cancel",
    label: "Cancel sequence input",
    description: "Cancel sequence input",
    type: "input",
    defaultValue: ["a", "b"],
  },
  {
    key: "input_sequence",
    label: "Input sequence",
    description: "Input sequence",
    type: "events"
  },
  {
    key: "input_sequence_completed",
    label: "Input sequence completed",
    description: "Input sequence completed",
    type: "events"
  },
];

export const compile = (input, helpers) => {
  const { _callNative, _rpn, _stackPushConst, _compileSubScript, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue, event } = helpers;
  const {input_sequence, input_cancel, input_sequence_completed} = input;
  
  const inputCancelValue = inputDec(input_cancel);
  
  const script_ref = _compileSubScript("input", input_sequence, "simulated_input_0");
  
  const completed_script_ref = _compileSubScript("input", input_sequence_completed, "simulated_input_complete_0");
  
  _addComment(`Simulated Inputs Attach`);
  
  _stackPushConst(`_${completed_script_ref}`);
  _stackPushConst(`___bank_${completed_script_ref}`);  
  _stackPushConst(`_${script_ref}`);
  _stackPushConst(`___bank_${script_ref}`);  
  _stackPushConst(inputCancelValue);
  _callNative("vm_attach_simulate_input");
  _stackPop(5);
  
};
