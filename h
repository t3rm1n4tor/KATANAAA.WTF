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
            -- Also try ClickDetector if present
            local cd = findClickDetector(diamondInst)
            if cd and typeof(fireclickdetector) == "function" then
                pcall(function() fireclickdetector(cd) end)
            end
        end, DIAMOND_TAKE_INTERVAL)
        task.wait()
    end
end

-- -------------- Teleport re-exec (your function) --------------
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

-- -------------- Server Hop (persistent, per-place, keeps trying) --------------
local function serverHop()
    local placeId = game.PlaceId
    local currentJobId = game.JobId
    local VISITED_FILE = "hopper_servers.json"
    local MAX_KEEP = 1000

    queueOnTeleportHello()

    local hasFS = (typeof(isfile) == "function" and typeof(readfile) == "function" and typeof(writefile) == "function")

    -- Read/write visited DB as a per-place map: { ["<placeId>"] = { "<jobId>", ... } }
    local function loadVisited()
        local db = {}
        if hasFS then
            if not isfile(VISITED_FILE) then
                pcall(writefile, VISITED_FILE, "{}")
            end
            local ok, s = pcall(readfile, VISITED_FILE)
            if ok and type(s) == "string" and #s > 0 then
                local ok2, data = pcall(function() return HttpService:JSONDecode(s) end)
                if ok2 and type(data) == "table" then
                    db = data
                end
            end
        end
        -- Back-compat: if file was a flat array, convert it under current placeId
        if #db > 0 then
            db = { [tostring(placeId)] = db }
        end

        local key = tostring(placeId)
        db[key] = db[key] or {}

        -- Build set/list for current place
        local set, list = {}, {}
        for _, id in ipairs(db[key]) do
            if type(id) == "string" and not set[id] then
                set[id] = true
                table.insert(list, id)
            end
        end
        return db, key, set, list
    end

    local function saveVisited(db, key, list)
        if not hasFS then return end
        -- trim
        while #list > MAX_KEEP do
            table.remove(list, 1)
        end
        db[key] = list
        pcall(function()
            writefile(VISITED_FILE, HttpService:JSONEncode(db))
        end)
    end

    local function httpGet(url)
        local ok, body = pcall(HttpService.GetAsync, HttpService, url, true)
        if ok and type(body) == "string" then return true, body end
        local req = (syn and syn.request) or request or http_request
        if req then
            local res = req({Url = url, Method = "GET"})
            if res and (res.StatusCode == 200 or res.StatusCode == 0) and type(res.Body) == "string" then
                return true, res.Body
            end
        end
        return false, nil
    end

    local db, dbKey, visitedSet, visitedList = loadVisited()

    -- Always record the current server to skip it
    if not visitedSet[currentJobId] then
        visitedSet[currentJobId] = true
        table.insert(visitedList, currentJobId)
        saveVisited(db, dbKey, visitedList)
    end

    -- Gather candidates repeatedly until we find some
    local function collectCandidates(maxPages)
        maxPages = maxPages or 40
        local function fetch(order)
            local cursor = nil
            local fresh = {}
            local fallback = {}
            local seen = {}
            for _ = 1, maxPages do
                local url = string.format(
                    "https://games.roblox.com/v1/games/%d/servers/Public?sortOrder=%s&limit=100%s",
                    placeId,
                    order,
                    cursor and ("&cursor=" .. HttpService:UrlEncode(cursor)) or ""
                )
                local ok, body = httpGet(url)
                if not ok then break end
                local ok2, data = pcall(function() return HttpService:JSONDecode(body) end)
                if not ok2 or not (data and data.data) then break end

                for _, s in ipairs(data.data) do
                    local id = s and s.id
                    local playing = tonumber(s.playing) or 0
                    local maxPlayers = tonumber(s.maxPlayers) or 0
                    if id and id ~= currentJobId and maxPlayers > 0 and playing < maxPlayers and not seen[id] then
                        seen[id] = true
                        if not visitedSet[id] then
                            table.insert(fresh, id) -- never visited in this place
                        else
                            table.insert(fallback, id) -- visited before, but not current
                        end
                    end
                end

                cursor = data.nextPageCursor
                if not cursor then break end
                task.wait(0.03)
            end
            return fresh, fallback
        end

        local f1, fb1 = fetch("Asc")
        local f2, fb2 = fetch("Desc")

        local combinedFresh, combinedFallback = {}, {}
        for _, v in ipairs(f1) do table.insert(combinedFresh, v) end
        for _, v in ipairs(f2) do table.insert(combinedFresh, v) end
        for _, v in ipairs(fb1) do table.insert(combinedFallback, v) end
        for _, v in ipairs(fb2) do table.insert(combinedFallback, v) end

        return combinedFresh, combinedFallback
    end

    -- Shuffle helper
    local function shuffle(t)
        for i = #t, 2, -1 do
            local j = math.random(1, i)
            t[i], t[j] = t[j], t[i]
        end
        return t
    end

    -- Attempt loop with retries, backoff, and TeleportInitFailed bounce
    local tried = {}
    local pending = false
    local lastAttemptTime = 0

    local function attemptFromList(list)
        for i = 1, #list do
            local id = list[i]
            if id and not tried[id] then
                tried[id] = true
                pending = true
                lastAttemptTime = tick()
                local ok = pcall(function()
                    TeleportService:TeleportToPlaceInstance(placeId, id, LocalPlayer)
                end)
                if ok then
                    -- Wait for teleport; TeleportInitFailed will trigger if it fails to init
                    return true
                else
                    -- immediate failure; continue to next candidate
                    pending = false
                end
            end
        end
        return false
    end

    -- If TeleportInitFailed fires, try next candidate immediately
    local tConn
    tConn = TeleportService.TeleportInitFailed:Connect(function(player)
        if player ~= LocalPlayer then return end
        pending = false
    end)

    task.spawn(function()
        local backoff = 1
        while true do
            -- Watchdog: if a call was made but no teleport nor init-failure for a while, reset pending
            if pending and (tick() - lastAttemptTime) > 20 then
                pending = false
            end

            if not pending then
                -- Fetch candidates
                local fresh, fallback = collectCandidates(60)
                local candidates = {}

                if #fresh > 0 then
                    candidates = shuffle(fresh)
                elseif #fallback > 0 then
                    candidates = shuffle(fallback)
                else
                    -- Nothing right now; trim some very old visited to widen pool and retry later
                    for _ = 1, 100 do
                        local old = table.remove(visitedList, 1)
                        if not old then break end
                        visitedSet[old] = nil
                    end
                    saveVisited(db, dbKey, visitedList)
                    task.wait(math.min(backoff, 10))
                    backoff = backoff * 1.5
                    goto continue
                end

                backoff = 1
                local started = attemptFromList(candidates)
                if not started then
                    -- All candidates already tried in this session; clear tried and refetch next loop
                    tried = {}
                    task.wait(0.5)
                else
                    -- Wait until either teleport happens or init-failed resets 'pending'
                    task.wait(0.5)
                end
            else
                task.wait(0.5)
            end
            ::continue::
        end
    end)
end

-- -------------- Coordinator --------------
local state = {
    processedChests = {},
    pauseForDiamond = false,
    serverHopStarted = false,
}

-- Initial death/HasDied check loop -> hop if true
while task.wait(0.5) do
    local ss = game.Players.LocalPlayer:GetAttribute("HasDied")
    if ss then
        state.serverHopStarted = true
        serverHop()
        print("hop")
        break
    elseif game.Players.LocalPlayer.Character ~= nil and ss == nil then
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
