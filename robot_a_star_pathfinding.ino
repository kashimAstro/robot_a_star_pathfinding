#include <pthread.h>
#include <Hash.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AccelStepper.h>
#include <ArduinoOTA.h>

const char* ssid     = "YOUR_WIFI_ESSID";
const char* password = "YOUR_WIFI_PASSWORD";

IPAddress staticIP(192, 168, 1, 132);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);

#define IN1_1 5   // GPIO 5 (D1)
#define IN2_1 4   // GPIO 4 (D2)
#define IN3_1 14  // GPIO 14 (D5)
#define IN4_1 12  // GPIO 12 (D6)

#define IN1_2 0   // GPIO 0 (D3)
#define IN2_2 2   // GPIO 2 (D4)
#define IN3_2 13  // GPIO 13 (D7)
#define IN4_2 15  // GPIO 15 (D8)


// 28BYJ-48 full-step 2048 passi per rivoluzione (per il gear ratio di 1:64)
// 28BYJ-48 half-step 4096 passi per rivoluzione 
// mode half-step con ULN2003 spesso IN1, IN3, IN2, IN4 oppure IN1, IN2, IN3, IN4.
AccelStepper stepper1(AccelStepper::HALF4WIRE, IN1_1, IN3_1, IN2_1, IN4_1);
AccelStepper stepper2(AccelStepper::HALF4WIRE, IN1_2, IN3_2, IN2_2, IN4_2);

ESP8266WebServer server(80);
int steps_per_revolution = 1000; //4096; // 2048 = full-step

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>A* robot</title>
    <style>
      body {
            font-family: Arial, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            background-color: #f0f0f0;
            margin: 0;
            padding: 20px;
            box-sizing: border-box;
        }
	#main-content {
            display: flex;
            width: 100%;
            max-width: 1200px;
            justify-content: center;
            gap: 30px; 
            flex-wrap: wrap;
        }
        #e_sidebar {
            padding:5px;
            border: 1px solid #ccc;
	    position:fixed;
	    left:0px;
	    top:0px;
            background-color: #e9e9e9;
            padding: 15px;
            border-radius: 8px;
            box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
            max-height: 80vh;
            overflow-y: auto;
	    display:none;
        }
        #e_sidebar h3 {
            margin-top: 0;
            color: #333;
        }
        .example-maze-thumbnail {
            border: 1px solid #ccc;
            margin-bottom: 10px;
            cursor: pointer;
            transition: transform 0.2s ease, border-color 0.2s ease;
            background-color: #fff;
        }
        .example-maze-thumbnail:hover {
            transform: translateY(-2px);
            border-color: #007bff;
        }
        .example-maze-thumbnail canvas {
            display: block; 
        }
        #controls {
            margin-bottom: 20px;
            text-align: center;
        }
        #controls input[type="file"],
        #controls input[type="number"] { 
            margin-bottom: 10px;
        }
	.input-with-icon {
            display: flex; 
            align-items: center; 
            gap: 5px; 
            margin-bottom: 10px; 
            width: 80%;
            max-width: 300px;
            justify-content: center; 
        }

        .input-with-icon input[type="number"] {
            flex-grow: 1; 
            margin-bottom: 0;
        }

        .input-with-icon .icon {
            width: 20px; 
            height: 20px;
            fill: #555; 
        }

        #maze-container {
            border: 2px solid #333;
            background-color: #fff;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }
        canvas {
            display: block;
            max-width: 100%; 
            height: auto;
        }
        button {
            margin-top: 20px;
            padding: 10px 20px;
            font-size: 16px;
            cursor: pointer;
            background-color: #007bff;
            color: white;
            border: none;
            border-radius: 5px;
            transition: background-color 0.3s ease;
        }
        button:hover {
            background-color: #0056b3;
        }
        .instructions {
            margin-bottom: 20px;
            /*text-align: center;*/
            font-size: 0.6em;
            color: #555;
        }
        .error-message {
            color: red;
            margin-top: 10px;
        }

#image_input {
    opacity: 0;
    position: absolute;
    z-index: -1;
    width: 0.1px; 
    height: 0.1px;
    overflow: hidden;
}

.custom-file-upload {
    display: inline-block; 
    padding: 10px 20px;
    font-size: 16px;
    cursor: pointer;
    background-color: #007bff;
    color: white; 
    border: none;
    border-radius: 5px; 
    transition: background-color 0.3s ease, transform 0.2s ease; 
    text-align: center;
    line-height: normal;
    margin-bottom: 10px; 
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.custom-file-upload:hover {
    background-color: #0056b3;
    transform: translateY(-1px);
}

#image_input:focus + .custom-file-upload {
    outline: 2px solid #0056b3; 
    outline-offset: 2px; 
}

#file_name_display {
    margin-top: 5px;
    font-size: 0.9em;
    color: #555;
    white-space: nowrap; 
    overflow: hidden; 
    text-overflow: ellipsis; 
    max-width: 250px; 
    display: inline-block; 
}

#controls {
    display: flex;
    flex-direction: column; 
    align-items: center; 
}

#controls input[type="text"] {
    padding: 8px;
    border: 1px solid #ccc;
    border-radius: 4px;
    width: 80%;
    max-width: 300px;
    box-sizing: border-box; 
}
    </style>
</head>
<body>
     <div id="e_sidebar" style="display:none;">
  	<h3>Example Mazes:</h3>
    </div>
    <div id="controls">
	<button id="hide_sidebar" >Show examples mazes</button>
	<br>
        <label for="image_input" class="custom-file-upload">
            Upload your image maze
	</label> 
        <a id="hide_show_instruct" style="text-decoration:underline;font-size:12px;color:silver;">Help</a>	
        <input type="file" id="image_input" accept="image/*">
        <!--<span id="file_name_display">Nessun file selezionato</span>-->
        <p class="error-message" id="error_message"></p>
    </div>

    <div id="maze-container">
        <canvas id="maze_canvas"></canvas>
    </div>
    <span>linear step</span>
    <input type="number" value="500" min="1" max="100000" id="step_lin"/>
    <span>rotation step</span>
    <input type="number" value="1150" min="1" max="100000" id="step_rot"/>

    <button id="solve_button" disabled>Resolve Maze</button>

    <div class="instructions" id="instructions" style="display:none;">
        <p>Upload an image to generate the maze. Colors are interpreted as follows:</p>
        <ul>
            <li>*White (RGB: 255, 255, 255): Obstacle / Wall</li>
            <li>*Red (RGB: 255, 0, 0): Starting Point</li>
            <li>*Green (RGB: 0, 255, 0): Ending Point</li>
            <li>*Any other color: Path / Empty Space</li>
        </ul>
        <p><b>Please ensure the image has a red starting point and a green ending point for proper functionality.</b></p>
    </div>

    <script>
        const canvas = document.getElementById('maze_canvas');
        const ctx = canvas.getContext('2d');
        const image_input = document.getElementById('image_input');
        const solve_button = document.getElementById('solve_button');
        const error_message = document.getElementById('error_message');
        const e_sidebar = document.getElementById('e_sidebar'); 
	const hide_sidebar = document.getElementById('hide_sidebar');
	const hide_show_instruct = document.getElementById('hide_show_instruct');
	const instruct = document.getElementById('instructions');
	const input_step_lin = document.getElementById('step_lin');
	const input_step_rot = document.getElementById('step_rot');

        const TILE_SIZE = 20; // tile size image-based mazes
        const THUMBNAIL_TILE_SIZE = 5; // tile size thumbnails

	const predefined_mazes = [
            {
                name: "Small Maze",
                maze: [
                    [1,1,1,1,1,1,1,1,1,1],
                    [1,0,0,0,1,0,0,0,0,1],
                    [1,1,1,0,1,0,1,1,0,1],
                    [1,0,0,0,0,0,1,0,0,1],
                    [1,0,1,1,1,1,1,0,1,1],
                    [1,0,0,0,0,0,0,0,0,1],
                    [1,1,1,1,1,1,1,1,0,1],
                    [1,0,0,0,0,0,0,0,0,1],
                    [1,0,1,1,1,1,1,1,1,1],
                    [1,1,1,1,1,1,1,1,1,1]
                ],
                start: { row: 1, col: 1 },
                end: { row: 7, col: 8 }
            },
            {
                name: "Medium Maze",
                maze: [
                    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,1,1,1,1,1,1,1,1,1,1,1,0,1],
                    [1,0,1,0,0,0,0,0,0,0,0,0,1,0,1],
                    [1,0,1,0,1,1,1,1,1,1,1,0,1,0,1],
                    [1,0,1,0,1,0,0,0,0,0,1,0,1,0,1],
                    [1,0,1,0,1,0,1,1,1,0,1,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1],
                    [1,0,1,0,0,0,1,0,1,0,0,0,1,0,1],
                    [1,0,1,1,1,0,1,0,1,1,1,0,1,0,1],
                    [1,0,0,0,0,0,1,0,0,0,0,0,1,0,1],
                    [1,0,1,1,1,1,1,1,1,1,1,1,1,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]
                ],
                start: { row: 1, col: 1 },
                end: { row: 13, col: 13 }
            },
            {
                name: "Zigzag Maze",
                maze: [
                    [1,1,1,1,1,1,1,1,1,1,1,1],
                    [1,0,0,0,0,0,0,0,0,0,0,1],
                    [1,1,1,1,1,1,1,1,1,1,0,1],
                    [1,0,0,0,0,0,0,0,0,1,0,1],
                    [1,0,1,1,1,1,1,1,0,1,0,1],
                    [1,0,1,0,0,0,0,0,0,1,0,1],
                    [1,0,1,0,1,1,1,1,1,1,0,1],
                    [1,0,1,0,0,0,0,0,0,0,0,1],
                    [1,0,1,1,1,1,1,1,1,1,1,1],
                    [1,0,0,0,0,0,0,0,0,0,0,1],
                    [1,1,1,1,1,1,1,1,1,1,1,1]
                ],
                start: { row: 1, col: 1 },
                end: { row: 9, col: 10 }
            },
            {
                name: "Spiral Maze",
                maze: [
                    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1],
                    [1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1],
                    [1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,1],
                    [1,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,1],
                    [1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1],
                    [1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1],
                    [1,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,1],
                    [1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,1],
                    [1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1],
                    [1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]
                ],
                start: { row: 1, col: 1 },
                end: { row: 8, col: 8 }
            },
            {
                name: "Large Sparse Maze",
                maze: [
                    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1],
                    [1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1],
                    [1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1],
                    [1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,1],
                    [1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,0,1],
                    [1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,1],
                    [1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,1,0,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,1,1,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,0,0,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,1,1,1,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,1,0,0,0,0,1,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1],
                    [1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1],
                    [1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1],
                    [1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1],
                    [1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1],
                    [1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1],
                    [1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1],
                    [1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1],
                    [1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1],
                    [1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1],
                    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]
                ],
                start: { row: 1, col: 1 },
                end: { row: 28, col: 28 }
            }
        ];

        let maze = [];
        let MAZE_ROWS = 0;
        let MAZE_COLS = 0;
        let START_NODE = null;
        let END_NODE = null;

	function load_maze(newMazeData, newStartNode, newEndNode) {
	    console.log(`new-maze: ${newMazeData} new-start: ${newStartNode} new-end: ${newEndNode}`);
            maze = newMazeData;
            MAZE_ROWS = maze.length;
            MAZE_COLS = maze[0].length;
            START_NODE = newStartNode;
            END_NODE = newEndNode;
            canvas.width = MAZE_COLS * TILE_SIZE;
            canvas.height = MAZE_ROWS * TILE_SIZE;
            draw_maze(maze, START_NODE, END_NODE, canvas, ctx, TILE_SIZE); 
            solve_button.disabled = false;
            error_message.textContent = ''; 
        }

	function render_predefined_mazes() {
            predefined_mazes.forEach((mazeData, index) => {
                const container = document.createElement('div');
                container.classList.add('example-maze-thumbnail');
                container.dataset.mazeIndex = index; // store index click 

                const title = document.createElement('h4');
                title.textContent = mazeData.name;
                title.style.textAlign = 'center';
                title.style.margin = '5px 0';
                container.appendChild(title);

                const thumbCanvas = document.createElement('canvas');
                // calculate dimensions for thumbnail
                thumbCanvas.width = mazeData.maze[0].length * THUMBNAIL_TILE_SIZE;
                thumbCanvas.height = mazeData.maze.length * THUMBNAIL_TILE_SIZE;
                
                const thumbCtx = thumbCanvas.getContext('2d');
                draw_maze(mazeData.maze, mazeData.start, mazeData.end, thumbCanvas, thumbCtx, THUMBNAIL_TILE_SIZE);
                container.appendChild(thumbCanvas);
                
                e_sidebar.appendChild(container);

                container.addEventListener('click', () => {
                    const selectedMaze = predefined_mazes[container.dataset.mazeIndex];
                    load_maze(selectedMaze.maze, selectedMaze.start, selectedMaze.end);
                });
            });
        }

        function draw_maze(mazeData, startNode, endNode, targetCanvas, targetCtx, tileSize) {
            targetCtx.clearRect(0, 0, targetCanvas.width, targetCanvas.height); // Clear canvas

            const rows = mazeData.length;
            const cols = mazeData[0].length;

            for (let r = 0; r < rows; r++) {
                for (let c = 0; c < cols; c++) {
                    targetCtx.fillStyle = mazeData[r][c] === 1 ? '#333' : '#fff'; 
                    if (startNode && r === startNode.row && c === startNode.col) {
                        targetCtx.fillStyle = 'green';
                    } else if (endNode && r === endNode.row && c === endNode.col) {
                        targetCtx.fillStyle = 'red';
                    }
                    targetCtx.fillRect(c * tileSize, r * tileSize, tileSize, tileSize);
                    targetCtx.strokeStyle = '#eee'; // Grid lines
                    targetCtx.strokeRect(c * tileSize, r * tileSize, tileSize, tileSize);
                }
            }
        }

        function draw_maze() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            for (let r = 0; r < MAZE_ROWS; r++) {
                for (let c = 0; c < MAZE_COLS; c++) {
                    ctx.fillStyle = maze[r][c] === 1 ? '#333' : '#fff'; 
                    if (START_NODE && r === START_NODE.row && c === START_NODE.col) {
                        ctx.fillStyle = 'green';
                    } else if (END_NODE && r === END_NODE.row && c === END_NODE.col) {
                        ctx.fillStyle = 'red';
                    }
                    ctx.fillRect(c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE);
                    ctx.strokeStyle = '#eee'; 
                    ctx.strokeRect(c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE);
                }
            }
        }

        function draw_cell(row, col, color) {
            ctx.fillStyle = color;
            ctx.fillRect(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            ctx.strokeStyle = '#eee';
            ctx.strokeRect(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }

        function draw_turn_text(row, col, text) {
            ctx.fillStyle = 'black';
            ctx.font = `${TILE_SIZE * 0.7}px Arial`; 
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText(text, col * TILE_SIZE + TILE_SIZE / 2, row * TILE_SIZE + TILE_SIZE / 2);
        }

        function esp_path_point(point, type, index) {
            const logMessage = `Step ${index}: (${point.row}, ${point.col}) - Type: ${type}`;
            console.log(logMessage); 
	    let step_lin = input_step_lin.value; // passi in avanti
            let step_rot = input_step_rot.value; // passi in rotazione
            let serverUrl = ""; 
            if(type == "PATH" || type == "START" || type == "END") serverUrl="/forward?mot1=1&mot2=1&step="+step_lin;
	    else if(type == "LEFT") serverUrl="/left?mot1=1&mot2=1&step="+step_rot;
	    else if(type == "RIGHT") serverUrl="/right?mot1=1&mot2=1&step="+step_rot;

	    console.log(serverUrl);

            if (serverUrl) {
                const payload = {
                    step: index,
                    row: point.row,
                    col: point.col,
                    type: type,
                    timestamp: new Date().toISOString() 
                };
		console.log(payload);
	        
                fetch(serverUrl, {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json', 
                    },
                    body: JSON.stringify(payload), 
                })
                .then(response => {
                    if (!response.ok) {
                        console.error(`Errore HTTP: ${response.status} per ${logMessage}`);
                    }
                })
                .catch(error => {
                    console.error(`Errore di fetch per ${logMessage}:`, error);
                });
            } else {
                console.warn("Nessun URL del server specificato. La chiamata HTTP non verrà effettuata.");
            }
        }

	function draw_path(path) {
            if (!path || path.length < 2) return; 
            for (let i = 0; i < path.length; i++) {
                const node = path[i];
                const prevNode = path[i - 1];
                const nextNode = path[i + 1];

                let cellColor = 'rgba(0, 123, 255, 0.7)'; 
                let turnText = '';
                let pointType = 'PATH';

                const isStart = (START_NODE && node.row === START_NODE.row && node.col === START_NODE.col);
                const isEnd = (END_NODE && node.row === END_NODE.row && node.col === END_NODE.col);

                if (isStart) {
                    pointType = 'START';
                } else if (isEnd) {
                    pointType = 'END';
                }

                if (!isStart && !isEnd) {
                    if (prevNode && nextNode) {
                        const vec1 = { dr: node.row - prevNode.row, dc: node.col - prevNode.col };
                        const vec2 = { dr: nextNode.row - node.row, dc: nextNode.col - node.col };
                        if (vec1.dr !== vec2.dr || vec1.dc !== vec2.dc) {
                            cellColor = 'orange'; 
                            const crossProduct = vec1.dc * vec2.dr - vec1.dr * vec2.dc;

                            if (crossProduct > 0) {
                                turnText = 'D'; 
                                pointType = 'RIGHT';
                            } else if (crossProduct < 0) {
                                turnText = 'S'; 
                                pointType = 'LEFT';
                            }
                        }
                    }
                    draw_cell(node.row, node.col, cellColor);
                    if (turnText) {
                        draw_turn_text(node.row, node.col, turnText);
                    }
                }
                esp_path_point(node, pointType, i);
            }
        }

        function load_image_and_convert_maze(event) {
            const file = event.target.files[0];
            if (!file) {
                return;
            }

            error_message.textContent = '';
            solve_button.disabled = true;

            const reader = new FileReader();
            reader.onload = function(e) {
                const img = new Image();
                img.onload = function() {
                    const tempCanvas = document.createElement('canvas');
                    const tempCtx = tempCanvas.getContext('2d');
                    tempCanvas.width = img.width;
                    tempCanvas.height = img.height;
                    tempCtx.drawImage(img, 0, 0);

                    const imageData = tempCtx.getImageData(0, 0, img.width, img.height);
                    const data = imageData.data; 

                    MAZE_ROWS = img.height;
                    MAZE_COLS = img.width;
                    maze = []; // reset maze
                    START_NODE = null;
                    END_NODE = null;

                    canvas.width = MAZE_COLS * TILE_SIZE;
                    canvas.height = MAZE_ROWS * TILE_SIZE;

                    let startFound = false;
                    let endFound = false;

                    for (let r = 0; r < MAZE_ROWS; r++) {
                        maze[r] = [];
                        for (let c = 0; c < MAZE_COLS; c++) {
                            const pixelIndex = (r * MAZE_COLS + c) * 4;
                            const red = data[pixelIndex];
                            const green = data[pixelIndex + 1];
                            const blue = data[pixelIndex + 2];
                            if (red === 255 && green === 255 && blue === 255) {
                                maze[r][c] = 1;
                            } else if (red === 255 && green === 0 && blue === 0) {
                                if (!startFound) {
                                    maze[r][c] = 0;
                                    START_NODE = { row: r, col: c };
                                    startFound = true;
                                } else {
                                    error_message.textContent = "Attenzione: Trovati più punti di partenza rossi. Verrà usato il primo trovato.";
                                    maze[r][c] = 0;
                                }
                            } else if (red === 0 && green === 255 && blue === 0) {
                                if (!endFound) {
                                    maze[r][c] = 0;
                                    END_NODE = { row: r, col: c };
                                    endFound = true;
                                } else {
                                    error_message.textContent = "Attenzione: Trovati più punti di arrivo verdi. Verrà usato il primo trovato.";
                                    maze[r][c] = 0; 
                                }
                            } else {
                                maze[r][c] = 0; 
                            }
                        }
                    }

                    if (!START_NODE) {
                        error_message.textContent = "Errore: Nessun punto di partenza rosso trovato nell'immagine.";
                        solve_button.disabled = true;
                        return;
                    }
                    if (!END_NODE) {
                        error_message.textContent = "Errore: Nessun punto di arrivo verde trovato nell'immagine.";
                        solve_button.disabled = true;
                        return;
                    }
                    draw_maze();
                    //load_maze(newMaze, newStart, newEnd);
                    solve_button.disabled = false; 
                };
                img.src = e.target.result;
            };
            reader.readAsDataURL(file);
        }

        // node class for A*
        class Node {
            constructor(row, col, g = 0, h = 0, parent = null) {
                this.row = row;
                this.col = col;
                this.g = g;
                this.h = h;
                this.f = g + h; 
                this.parent = parent; 
            }

            equals(other) {
                return this.row === other.row && this.col === other.col;
            }
        }

        // heuristic function (Manhattan distance for a grid)
        function heuristic(node, endNode) {
            return Math.abs(node.row - endNode.row) + Math.abs(node.col - endNode.col);
        }

        function a_star(start, end) {
            const openList = []; 
            const closedList = new Set(); 
            const startNode = new Node(start.row, start.col, 0, heuristic(start, end));
            openList.push(startNode);

            const nodeMap = new Map(); 
            nodeMap.set(`${start.row},${start.col}`, startNode);

            while (openList.length > 0) {
                openList.sort((a, b) => a.f - b.f);
                const currentNode = openList.shift();

                if (currentNode.equals(end)) {
                    let path = [];
                    let temp = currentNode;
                    while (temp !== null) {
                        path.push({ row: temp.row, col: temp.col });
                        temp = temp.parent;
                    }
                    return path.reverse();
                }

                closedList.add(`${currentNode.row},${currentNode.col}`);

                const neighbors = [
                    { dr: -1, dc: 0 }, // Up
                    { dr: 1, dc: 0 },  // Down
                    { dr: 0, dc: -1 }, // Left
                    { dr: 0, dc: 1 }   // Right
                ];

                for (const move of neighbors) {
                    const newRow = currentNode.row + move.dr;
                    const newCol = currentNode.col + move.dc;

                    if (newRow >= 0 && newRow < MAZE_ROWS &&
                        newCol >= 0 && newCol < MAZE_COLS &&
                        maze[newRow][newCol] === 0) { // 0 means traversable
                        
                        const neighborCoords = `${newRow},${newCol}`;

                        if (closedList.has(neighborCoords)) {
                            continue;
                        }

                        const tentativeG = currentNode.g + 1;

                        let neighborNode = nodeMap.get(neighborCoords);

                        if (!neighborNode || tentativeG < neighborNode.g) {
                            if (!neighborNode) {
                                neighborNode = new Node(newRow, newCol);
                                nodeMap.set(neighborCoords, neighborNode);
                            }
                            neighborNode.g = tentativeG;
                            neighborNode.h = heuristic(neighborNode, end);
                            neighborNode.f = neighborNode.g + neighborNode.h;
                            neighborNode.parent = currentNode;

                            if (!openList.some(node => node.equals(neighborNode))) {
                                openList.push(neighborNode);
                            }
                        }
                    }
                }
            }

            return null; // no path found
        }

        image_input.addEventListener('change', load_image_and_convert_maze);
        solve_button.addEventListener('click', () => {
            if (!START_NODE || !END_NODE) {
                error_message.textContent = "Impossibile risolvere: i punti di partenza/arrivo non sono stati definiti correttamente dall'immagine.";
                return;
            }
            //draw_maze(); 
            drawMaze(maze, START_NODE, END_NODE, canvas, ctx, TILE_SIZE); // Redraw maze to clear previous path if any

            const path = a_star(START_NODE, END_NODE);
            if (path) {
                draw_path(path);
            } else {
                error_message.textContent = 'Nessun percorso trovato!';
            }
        });

        hide_sidebar.addEventListener('click', () => {
	    if(e_sidebar.style.display === 'none') {
        	e_sidebar.style.display = 'block';
    	    } else {
       		e_sidebar.style.display = 'none';
    	    }
        });

	hide_show_instruct.addEventListener('click', () => {
            if(instruct.style.display === 'none') {
                instruct.style.display = 'block';
            } else {
                instruct.style.display = 'none';
            }
        });

	render_predefined_mazes();
        if (predefined_mazes.length > 0) {
            load_maze(predefined_mazes[0].maze, predefined_mazes[0].start, predefined_mazes[0].end);
        } else {
            solve_button.disabled = true;
        }
    </script>
</body>
</html>
)rawliteral";

void handle_root() {
  server.send(200, "text/html", HTML_PAGE);
}


void handle_step() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("step"))
     steps_per_revolution = server.arg("step").toInt();

  server.send(200, "text/plain", "step received: " + String(steps_per_revolution));
}

void handle_left() {
  if (server.hasArg("step"))
     steps_per_revolution = server.arg("step").toInt();

  if (server.hasArg("mot1"))
     stepper1.move(-steps_per_revolution);
  if (server.hasArg("mot2"))
     stepper2.move(-steps_per_revolution);

  while(stepper1.distanceToGo() != 0 && stepper2.distanceToGo() != 0) {
      stepper1.run();
      stepper2.run();
      delay(1);
    }

  server.send(200, "text/plain", "left step: " + String(steps_per_revolution));
}

void handle_right() {
  if (server.hasArg("step"))
     steps_per_revolution = server.arg("step").toInt();

  if (server.hasArg("mot1"))
     stepper1.move(steps_per_revolution);
  if (server.hasArg("mot2"))
     stepper2.move(steps_per_revolution);

  while(stepper1.distanceToGo() != 0 && stepper2.distanceToGo() != 0) {
      stepper1.run();
      stepper2.run();
      delay(1);
    }

  server.send(200, "text/plain", "right step: " + String(steps_per_revolution));
}

void handle_forward() {
  if (server.hasArg("step"))
     steps_per_revolution = server.arg("step").toInt();

  if (server.hasArg("mot1"))
     stepper1.move(steps_per_revolution);
  if (server.hasArg("mot2"))
     stepper2.move(-steps_per_revolution);

  while(stepper1.distanceToGo() != 0 && stepper2.distanceToGo() != 0) {
      stepper1.run();
      stepper2.run();
      delay(1);
    }

  server.send(200, "text/plain", "up step: " + String(steps_per_revolution));
}

void handle_backward() {
  if (server.hasArg("step"))
     steps_per_revolution = server.arg("step").toInt();

  if (server.hasArg("mot1"))
     stepper1.move(-steps_per_revolution);
  if (server.hasArg("mot2"))
     stepper2.move(steps_per_revolution);

  while(stepper1.distanceToGo() != 0 && stepper2.distanceToGo() != 0) {
      stepper1.run();
      stepper2.run();
      delay(1);
    }

  server.send(200, "text/plain", "down step: " + String(steps_per_revolution));
}

void handle_stop() {
  stepper1.stop();
  stepper2.stop();
  server.send(200, "text/plain", "stop all mot");
}

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connessione a ");
  Serial.println(ssid);
  WiFi.mode(WIFI_AP_STA);
  if (WiFi.config(staticIP, gateway, subnet, dns, dns) == false) {
     Serial.println("Configuration failed.");
  }
  WiFi.begin(ssid, password);
  int c=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(c > 500) break;
  }

  Serial.println("");
  Serial.println("WiFi connesso");
  Serial.println("Indirizzo IP: ");
  Serial.println(WiFi.localIP());

  //ArduinoOTA.setPort(8552);
  ArduinoOTA.setHostname("robot001");
  ArduinoOTA.setPassword("1234");

  ArduinoOTA.onStart([]() {
    Serial.println("Inizio aggiornamento OTA...");
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Aggiornamento: " + type);
    server.stop();
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nAggiornamento OTA completato!");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Errore OTA[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
  });

  ArduinoOTA.begin();

  stepper1.setMaxSpeed(1000.0); 
  stepper1.setAcceleration(500); 

  stepper2.setMaxSpeed(1000.0); 
  stepper2.setAcceleration(500); 

  server.on("/", handle_root);
  server.on("/forward", handle_forward);
  server.on("/backward", handle_backward);
  server.on("/left", handle_left);
  server.on("/right", handle_right);
  server.on("/stop", handle_stop);
  server.on("/stepall", handle_step);
  server.begin();
  Serial.println("Server HTTP avviato");
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  stepper1.run();
  stepper2.run();
}

