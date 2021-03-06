local function getAnimation(lookup, ds)
   if ds.x > 0 and ds.x >= math.abs(ds.y) then return lookup['right']
   elseif ds.x < 0 and ds.x <= -math.abs(ds.y) then return lookup['left']
   end
   if ds.y > 0 and ds.y > math.abs(ds.x) then return lookup['down']
   elseif ds.y < 0 and ds.y < -math.abs(ds.x) then return lookup['up']
   end
end

local walkAnims = {
   up = 'PriestWalkUp',
   down = 'PriestWalkDown',
   right = 'PriestWalkRight',
   left = 'PriestWalkLeft'
}
local idleAnims = {
   PriestWalkUp = 'PriestIdleUp',
   PriestWalkDown = 'PriestIdleDown',
   PriestWalkRight = 'PriestIdleRight',
   PriestWalkLeft = 'PriestIdleLeft'
}

local function enter(entity)
   entity.data.animator.animation = 'PriestIdleDown'
end

local function execute(entity, dt)
   local animator = entity.data.animator

   local vel = mulVec(entity.data.speed, normalizeVec(entity.data.heading))
   entity.rigidBody:setVelocity(vel)

   local anim = getAnimation(walkAnims, vel) or idleAnims[animator.animation]

   if anim and anim ~= animator.animation then
      animator.animation = anim
   end

   return entity.data.pendingState
end

local function exit(entity)
   entity.data.pendingState = nil
end

local function onMessage(entity, telegram)
   local v = telegram.info
   if v.axis then
      if v.axis == Axis.X then entity.data.heading.x = v.val end
      if v.axis == Axis.Y then entity.data.heading.y = v.val end
   else
      if v.x then entity.data.heading.x = entity.data.heading.x + v.x end
      if v.y then entity.data.heading.y = entity.data.heading.y + v.y end
   end

   if v.msg == 'attack' then
      entity.data.pendingState = AttackState
   end
   --entity.data.velocity = addVec(entity.data.velocity, Vec(telegram.info.x, telegram.info.y))
end

MyState = {
   enter = enter,
   execute = execute,
   exit = exit,
   onMessage = onMessage
}

return MyState
