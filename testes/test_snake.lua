-- PyLua Professional Snake Game
-- Uses the new engine.Screen system for Scene Management

import engine

print("Initializing Snake Game with Screen System...")

-- 1. Global Setup
win = engine.Window.create("PyLua Snacks XL", 640, 640)
math.randomseed(os.time())

GRID_SIZE = 20
TILE_SIZE = 32

-- =========================================================================
-- HOME SCREEN
-- =========================================================================
MenuScreen = {
    onEnter = function(self) {
        print("Entered Main Menu")
    },
    
    update = function(self, dt) {
        -- No logic needed for static menu
    },
    
    draw = function(self) {
        engine.Graphics.clear(0.05, 0.05, 0.1, 1.0)
        engine.UI.panel(120, 160, 400, 320, 0.1, 0.1, 0.2, 0.9)
        engine.Graphics.drawText("PYLUA SNACKS", 160, 200, 4, 0, 1, 0, 1)
        
        if (engine.UI.button("START GAME", 220, 300, 200, 60)) {
            engine.Screen.set(GameScreen)
        }
        
        if (engine.UI.button("EXIT", 220, 380, 200, 60)) {
            engine.stop()
        }
    }
}

-- =========================================================================
-- GAME SCREEN
-- =========================================================================
GameScreen = {
    onEnter = function(self) {
        self.snake = {{x=10, y=10}, {x=10, y=11}, {x=10, y=12}}
        self.dir = {x=0, y=-1}
        self.nextDir = {x=0, y=-1}
        self.food = {x=math.random(0, GRID_SIZE-1), y=math.random(0, GRID_SIZE-1)}
        self.score = 0
        self.gameOver = false
        self.timer = 0
        self.moveDelay = 0.15
        print("Game Started!")
    },
    
    update = function(self, dt) {
        if (self.gameOver) {
            if (engine.Input.isKeyJustPressed("enter")) {
                self:onEnter() -- Reset
            }
            if (engine.Input.isKeyJustPressed("escape")) {
                engine.Screen.set(MenuScreen)
            }
            return
        }

        -- Handle Input
        if (engine.Input.isKeyDown("up") and self.dir.y != 1)    { self.nextDir = {x=0, y=-1} }
        if (engine.Input.isKeyDown("down") and self.dir.y != -1)  { self.nextDir = {x=0, y=1} }
        if (engine.Input.isKeyDown("left") and self.dir.x != 1)  { self.nextDir = {x=-1, y=0} }
        if (engine.Input.isKeyDown("right") and self.dir.x != -1) { self.nextDir = {x=1, y=0} }

        self.timer = self.timer + dt
        if (self.timer >= self.moveDelay) {
            self.timer = 0
            self.dir = self.nextDir
            
            head = self.snake[1]
            newHead = {x = head.x + self.dir.x, y = head.y + self.dir.y}
            
            if (newHead.x < 0 or newHead.x >= GRID_SIZE or newHead.y < 0 or newHead.y >= GRID_SIZE) {
                self.gameOver = true
                return
            }
            
            for i, segment in ipairs(self.snake) {
                if (newHead.x == segment.x and newHead.y == segment.y) {
                    self.gameOver = true
                    return
                }
            }
            
            table.insert(self.snake, 1, newHead)
            if (newHead.x == self.food.x and newHead.y == self.food.y) {
                self.score = self.score + 10
                self.food = {x=math.random(0, GRID_SIZE-1), y=math.random(0, GRID_SIZE-1)}
            } else {
                table.remove(self.snake)
            }
        }
    },
    
    draw = function(self) {
        engine.Graphics.clear(0.05, 0.05, 0.1, 1.0)
        
        -- Grid
        for i=0, GRID_SIZE do
            engine.Graphics.drawLine(i*TILE_SIZE, 0, i*TILE_SIZE, 640, 1, 0.1, 0.1, 0.2, 1)
            engine.Graphics.drawLine(0, i*TILE_SIZE, 640, i*TILE_SIZE, 1, 0.1, 0.1, 0.2, 1)
        }

        -- Food
        engine.Graphics.drawRect(self.food.x*TILE_SIZE + 4, self.food.y*TILE_SIZE + 4, TILE_SIZE - 8, TILE_SIZE - 8, 1.0, 0.2, 0.2, 1.0)
        
        -- Snake
        for i, segment in ipairs(self.snake) {
            r = 0.2; g = 0.8; b = 0.2
            if (i == 1) { r = 0.3; g = 1.0; b = 0.3 }
            engine.Graphics.drawRect(segment.x*TILE_SIZE + 2, segment.y*TILE_SIZE + 2, TILE_SIZE - 4, TILE_SIZE - 4, r, g, b, 1.0)
        }
        
        -- Score
        engine.Graphics.drawText("SCORE: " .. self.score, 20, 20, 3, 1, 1, 1, 1)
        
        if (self.gameOver) {
            engine.Graphics.drawRect(0, 0, 640, 640, 0, 0, 0, 0.7)
            engine.Graphics.drawText("GAME OVER", 160, 280, 6, 1, 0, 0, 1)
            engine.Graphics.drawText("ENTER: RESTART | ESC: MENU", 100, 360, 2, 1, 1, 1, 1)
        }
    }
}

-- =========================================================================
-- ENGINE START
-- =========================================================================
engine.Screen.set(MenuScreen)

engine.run({
    window = win,
    init = function() {
        print("Engine Running")
    }
})
