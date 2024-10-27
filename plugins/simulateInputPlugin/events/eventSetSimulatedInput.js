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

export const id = "EVENT_SET_SIMULATED_INPUT";
export const name = "Set simulated input";
export const groups = ["EVENT_GROUP_INPUT"];

export const autoLabel = (fetchArg) => {
  return `Set simulated input`;
};

export const fields = [  
  {
    key: "input",
    label: "Input",
    description: "Input",
    type: "input",
    defaultValue: [],
  }
];

export const compile = (input, helpers) => {
  const { _callNative, _stackPushConst, _stackPop, _addComment } = helpers;
  
  const inputValue = inputDec(input.input);
    
  _addComment(`Set Simulated Input`);
  
  _stackPushConst(inputValue);
  _callNative("vm_set_simulated_input");
  _stackPop(1);
  
};
