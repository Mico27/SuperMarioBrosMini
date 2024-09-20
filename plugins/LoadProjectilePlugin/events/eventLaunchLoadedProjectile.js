
// Based on shin's youtube: https://www.youtube.com/watch?v=qcWO1V6XU5A
// Prerelease
const id = "FO_EVENT_LAUNCH_LOADED_PROJECTILE";
const name = "Launch Loaded Projectile";
const groups = ["Projectiles"];

const fields = [
  {
    type: "group",
    fields: [
      {
        key: "spriteSheetId",
        type: "sprite",
        label: "Sprite Sheet",
        defaultValue: "LAST_SPRITE",
        width: "50%",
      },
      {
        key: "slot",
            type: "number",
            label: "Projectile Slot",
            defaultValue: 0,
            type: "select",
            options: [
              [0, "0"],
              [1, "1"],
              [2, "2"],
              [3, "3"],
              [4, "4"],
            ],
            width: "50%",
        },
    ],
  },

  {
    type: "group",
    fields: [
      {
        key: "source",
        label: "Projectile Source",
        type: "select",
        options: [
          [0, "From Another Scene"],
          [1, "From File"],
        ],
        defaultValue: 0,
    },
    {
		key: "fileName",
        type: "text",
        label: "Scene Name",
        conditions: 
        [{
            key: "source",
            eq: 1
        }]
    },
    {
        key: "scene",
        label: "Scene",
        type: "scene",
        flexBasis: "100%",
        defaultValue: "LAST_SCENE",
        conditions: 
        [{
            key: "source",
            ne: 1
        }]
    },
    ],
  },

  {
    key: "actorId",
    type: "actor",
    label: "Source",
    defaultValue: "$self$",
  },
    {
      type: "group",
      fields: [
        {
          key: "x",
          label: "X Offset",
          type: "number",
          min: -256,
          max: 256,
          width: "50%",
          defaultValue: 0,
        },
        {
          key: "y",
          label: "Y Offset",
          type: "number",
          min: -256,
          max: 256,
          width: "50%",
          defaultValue: 0,
        },
      ],
    },
    {
      type: "group",
      fields: [
        {
          label: "Launch at",
          key: "directionType",
          type: "select",
          options: [
            ["direction", "Fixed Direction"],
            ["actor", "Actor Direction"],
            ["target", "Target Actor"],
            ["angle", "Angle"],
            ["anglevar", "Angle Variable"],
          ],
          defaultValue: "direction",
          alignBottom: true,
        },
        {
          key: "otherActorId",
          label: "Direction",
          type: "actor",
          defaultValue: "$self$",
          conditions: [
            {
              key: "directionType",
              eq: "actor",
            },
          ],
        },
        {
          key: "direction",
          label: "Direction",
          type: "direction",
          defaultValue: "right",
          conditions: [
            {
              key: "directionType",
              eq: "direction",
            },
          ],
        },
        {
          key: "angle",
          label: "Angle",
          type: "angle",
          defaultValue: 0,
          min: -256,
          max: 256,
          conditions: [
            {
              key: "directionType",
              eq: "angle",
            },
          ],
        },
        {
          key: "angleVariable",
          label: "Angle",
          type: "variable",
          defaultValue: "LAST_VARIABLE",
          conditions: [
            {
              key: "directionType",
              eq: "anglevar",
            },
          ],
        },
        {
          key: "targetActorId",
          label: "Target",
          type: "actor",
          defaultValue: "$self$",
          conditions: [
            {
              key: "directionType",
              eq: "target",
            },
          ],
        },
      ],
    },
    {
      type: "group",
      alignBottom: true,
      fields: [
        {
          key: "loopAnim",
          label: "Loop Animation",
          type: "checkbox",
          defaultValue: true,
        },
        {
          key: "destroyOnHit",
          label: "Destroy On Hit",
          type: "checkbox",
          defaultValue: true,
        },
      ],
    },
];


const compile = (input, helpers) => {
    const {
      launchProjectileInDirection,
      launchProjectileInAngle,
      launchProjectileInSourceActorDirection,
      launchProjectileInActorDirection,
      launchProjectileInAngleVariable,
      launchProjectileTowardsActor,
      actorSetActive,
      appendRaw,
      scenes,
      warnings,
      _addComment,
    } = helpers;

    const projectileIndex = input.slot;
    let sceneName;

    if(input.source === 0){
        sceneName = scenes.find((s) => s.id === input.scene).symbol + "_projectiles";
    } else {
        sceneName = input.fileName;
        if (input.tilemap === undefined) warnings("Did you remember to write the filename of the projectile?");
    }
    _addComment("Load Projectile Type");
    appendRaw(`VM_PROJECTILE_LOAD_TYPE ${projectileIndex}, ___bank_${sceneName}, _${sceneName}`);
  
    actorSetActive(input.actorId);
  
    if (input.directionType === "direction") {
      launchProjectileInDirection(
        projectileIndex,
        input.x,
        input.y,
        input.direction,
        input.destroyOnHit,
        input.loopAnim
      );
    } else if (input.directionType === "angle") {
      launchProjectileInAngle(
        projectileIndex,
        input.x,
        input.y,
        input.angle,
        input.destroyOnHit,
        input.loopAnim
      );
    } else if (input.directionType === "anglevar") {
      launchProjectileInAngleVariable(
        projectileIndex,
        input.x,
        input.y,
        input.angleVariable,
        input.destroyOnHit,
        input.loopAnim
      );
    } else if (input.directionType === "actor") {
      if (input.actorId === input.otherActorId) {
        launchProjectileInSourceActorDirection(
          projectileIndex,
          input.x,
          input.y,
          input.destroyOnHit,
          input.loopAnim
        );
      } else {
        launchProjectileInActorDirection(
          projectileIndex,
          input.x,
          input.y,
          input.otherActorId,
          input.destroyOnHit,
          input.loopAnim
        );
      }
    } else if (input.directionType === "target") {
      if (input.actorId === input.targetActorId) {
        launchProjectileInSourceActorDirection(
          projectileIndex,
          input.x,
          input.y,
          input.destroyOnHit,
          input.loopAnim
        );
      } else {
        launchProjectileTowardsActor(
          projectileIndex,
          input.x,
          input.y,
          input.targetActorId,
          input.destroyOnHit,
          input.loopAnim
        );
      }
    }
};

module.exports = {
    id,
    name,
    groups,
    fields,
    compile,
    waitUntilAfterInitFade: true,
};