require 'assets.scripts.MyState'

local function init(entity, game)
   entity.data.heading = Vec(0, 0)
   entity.data.lastDs = Vec(0, 0)
   entity.data.speed = 48

   game:loadSpritesheet('assets/spritesheets/priest/priest.xml')
   entity.animation = 'PriestIdleDown'
   entity:initMachine(MyState)
end

Entity = {
   init = init
}

return Entity
