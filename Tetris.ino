//* Includes, Defines
#pragma region Includes, Defines

#include <DigitalIO.h>
#include <PsxControllerBitBang.h>
#include <MemoryFree.h>

#include <limits.h>
#include <FastLED.h>

//?#define _DEBUG
//?#define _AI_TRAINING_MODE
#define _AI_TRAINING_MAX_TETROMINOS 500
#define _SERIAL_CONTACT 'A'
#define _SERIAL_GAME_START 'S'
#define _SERIAL_GAME_END 'E'
#define _SERIAL_WEIGHT_VALUE 'W'
#define _SERIAL_TETROMINOS_VALUE 'B'

// These can be changed freely when using the bitbanged protocol
const byte PIN_PS2_ATT = 9;
const byte PIN_PS2_CMD = 10;
const byte PIN_PS2_DAT = 11;
const byte PIN_PS2_CLK = 12;

#define AI_FRAMES_PER_INPUT 5

#define DISPLAY_HOLD_WIDTH 4
#define DISPLAY_HOLD_HEIGHT 2
#define DISPLAY_HOLD_OFFSET_INDEX 0
#define DISPLAY_BOARD_WIDTH 10
#define DISPLAY_BOARD_HEIGHT 22
#define DISPLAY_BOARD_OFFSET_INDEX 8
#define DISPLAY_QUEUE_WIDTH 4
#define DISPLAY_QUEUE_HEIGHT 18
#define DISPLAY_QUEUE_OFFSET_INDEX 228

const int displayMainWidth = (DISPLAY_HOLD_WIDTH + DISPLAY_BOARD_WIDTH + DISPLAY_QUEUE_WIDTH);

#define LED_PIN 5
#define NUM_LEDS 300
#define BRIGHTNESS 50
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define FRAMERATE 60
const int restartAfterGameoverAsAI_frames = FRAMERATE * 10;

#define TETROMINO_SIZE 4
#define TETROMINO_COUNT 7

#define VANISHINGLINE 20
#define LINESPERLEVEL 10
#define MAXLEVEL 15
#define QUEUESIZE 6
#define WAITTOSTARTFRAMES 60

//ALTERNATE SPEED RATES
#define LOCKOUTRATE 30
#define LINECLEAREDRATE 10
#define SOFTDROPMODIFIER 20

//SCORE SCALING PER LEVEL (SSPL)
#define SSPL_SINGLE 100
#define SSPL_DOUBLE 300
#define SSPL_TRIPLE 500
#define SSPL_TETRIS 800
#define SSPL_MINITSPIN 100
#define SSPL_MINITSPIN_SINGLE 200
#define SSPL_MINITSPIN_DOUBLE 400
#define SSPL_TSPIN 400
#define SSPL_TSPIN_SINGLE 800
#define SSPL_TSPIN_DOUBLE 1200
#define SSPL_TSPIN_TRIPLE 1600

//SCORE SCALING PER CELL DROP (SSPCD)
#define SSPCD_SOFTDROP 1
#define SSPCDMAX_SOFTDROP 20
#define SSPCD_HARDDROP 2
#define SSPCDMAX_HARDDROP 40

//INPUT
#define DELAYEDAUTOSHIFT 30
#define AUTOREPEATRATE 10
#define MAXINFINITYSTEPS 15

//TSPINS, TRICKS, AND BACK TO BACK SCORING
#define BACKTOBACK_MOD 1.5f

#define KICKTABLE_SIZE 4

#define GHOSTFADE_MIN .25
#define GHOSTFADE_MAX .35
#define GHOSTFADE_SPEED .0025
#define GHOSTFADE_FRAMES 2
#define GHOSTFADE_DESATURATION .75

#define LINECLEAR_FADE .12

#define TOPOUT_DESATURATION .75
#define TOPOUT_BRIGHTNESS .3

#define I_SHAPE BasicShape(new point[4]{point(0, 2), point(1, 2), point(2, 2), point(3, 2)}, CRGB(0, 255, 255), I)
#define J_SHAPE BasicShape(new point[4]{point(0, 2), point(0, 1), point(1, 1), point(2, 1)}, CRGB(0, 0, 255), J)
#define L_SHAPE BasicShape(new point[4]{point(2, 2), point(0, 1), point(1, 1), point(2, 1)}, CRGB(255, 127, 0), L)
#define S_SHAPE BasicShape(new point[4]{point(1, 2), point(2, 2), point(0, 1), point(1, 1)}, CRGB(0, 255, 0), S)
#define Z_SHAPE BasicShape(new point[4]{point(0, 2), point(1, 2), point(1, 1), point(2, 1)}, CRGB(255, 0, 0), Z)
#define T_SHAPE BasicShape(new point[4]{point(1, 2), point(0, 1), point(1, 1), point(2, 1)}, CRGB(128, 0, 128), T)
#define O_SHAPE BasicShape(new point[4]{point(0, 0), point(0, 1), point(1, 0), point(1, 1)}, CRGB(255, 255, 0), O)

#pragma endregion

//* Enums
#pragma region Enums

enum TSpinType : uint8_t
{
    none,
    mini,
    full
};

enum Trick : uint8_t
{
    //This first line includes all tricks that allow back to back scoring
    miniTSpin_double,
    tetris,
    tSpin_single,
    tSpin_double,
    tSpin_triple,
    //This second line of tricks DO NOT allow back to back scoring
    singleLine,
    doubleLine,
    tripleLine,
    tSpin,
    miniTSpin,
    miniTSpin_single,
    noTrick
};

enum ShapeType : uint8_t
{
    T,
    J,
    L,
    I,
    S,
    Z,
    O
};

#pragma endregion

//* Point struct
struct point
{
    int8_t x, y;
    point(int x, int y) : x(x), y(y){};
    point() : x(0), y(0){};

    //Addition
    point operator+=(point pnt)
    {
        (*this).x += pnt.x;
        (*this).y += pnt.y;
        return (*this);
    }
    point operator+=(float num)
    {
        (*this).x += num;
        (*this).y += num;
        return (*this);
    }
    point operator+(point pnt) { return point((*this).x + pnt.x, (*this).y + pnt.y); }
    point operator+(float num) { return point((*this).x + num, (*this).y + num); }

    //Subtraction
    point operator-=(point pnt)
    {
        (*this).x -= pnt.x;
        (*this).y -= pnt.y;
        return (*this);
    }
    point operator-=(float num)
    {
        (*this).x -= num;
        (*this).y -= num;
        return (*this);
    }
    point operator-(point pnt) { return point((*this).x - pnt.x, (*this).y - pnt.y); }
    point operator-(float num) { return point((*this).x - num, (*this).y - num); }

    //Multiplication
    point operator*=(point pnt)
    {
        (*this).x *= pnt.x;
        (*this).y *= pnt.y;
        return (*this);
    }
    point operator*=(float num)
    {
        (*this).x *= num;
        (*this).y *= num;
        return (*this);
    }
    point operator*(point pnt) { return point((*this).x * pnt.x, (*this).y * pnt.y); }
    point operator*(float num) { return point((*this).x * num, (*this).y * num); }

    //Division
    point operator/=(point pnt)
    {
        (*this).x /= pnt.x;
        (*this).y /= pnt.y;
        return (*this);
    }
    point operator/=(float num)
    {
        (*this).x /= num;
        (*this).y /= num;
        return (*this);
    }
    point operator/(point pnt) { return point((*this).x / pnt.x, (*this).y / pnt.y); }
    point operator/(float num) { return point((*this).x / num, (*this).y / num); }

    //Equal (Assignment)
    point operator=(point pnt)
    {
        (*this).x = pnt.x;
        (*this).y = pnt.y;
        return (*this);
    }
    point operator=(float num)
    {
        (*this).x = num;
        (*this).y = num;
        return (*this);
    }
};

//* Function Declarations
#pragma region Function Declarations

point *getTable(int currentState, int direction, bool isIShape);
bool withinBounds(point cell);
bool isOccupied(point cell);
int countClearedRows();
point getCenter(point size);
bool spaceIsFree(point cell, point ignoredCells[TETROMINO_SIZE]);
point getShapeSize(point cells[TETROMINO_SIZE]);
void onAISpawn();

#pragma endregion

//*Variables
#pragma region Variables

#ifdef _AI_TRAINING_MODE
bool ai_waitForNewWeights = true;
bool builtInLED_State = true;
int ai_gameEndFrame = 0;
#endif

int total_linesCleared = 0;
int total_tetrominos = 0;

PsxControllerBitBang<PIN_PS2_ATT, PIN_PS2_CMD, PIN_PS2_DAT, PIN_PS2_CLK> psx;

bool controlledByAI = true;
int lastAIFrame = -1;

#ifdef _AI_TRAINING_MODE
float weight_aggregateHeight = -0.6644016;
float weight_completeLines = 0.42998326;
float weight_holes = -0.5571139;
float weight_bumpiness = -0.2516129;
#else
const float weight_aggregateHeight = -0.6644016;
const float weight_completeLines = 0.42998326;
const float weight_holes = -0.5571139;
const float weight_bumpiness = -0.2516129;
#endif

const uint8_t ai_maxRotationsForShapeType[] = {4, 4, 4, 2, 2, 2, 1};

const char *trick_strings[] = {
    "Mini T-Spin Double",
    "Tetris",
    "T-Spin Single",
    "T-Spin Double",
    "T-Spin Triple",
    "Single",
    "Double",
    "Triple",
    "T-Spin",
    "Mini T-Spin",
    "Mini T-Spin Single",
    ""};

CRGB leds[NUM_LEDS];

//Tricks equal or under this integer value allow back to back scoring
const uint8_t trickBackToBackCap = static_cast<Trick>(tSpin_triple); //tSpin_triple.ordinal();

const point tSpinCorners[] = {point(-1, 1), point(1, 1), point(1, -1), point(-1, -1)};

//Comparative operators:
bool operator==(point a, point b) { return a.x == b.x && a.y == b.y; }
bool operator==(point a, float num) { return a.x == num && a.y == num; }
bool operator!=(point a, point b) { return !(a == b); }
bool operator!=(point a, float num) { return !(a.x == num && a.y == num); }

const float frameSeconds = (float)1 / FRAMERATE;
const int frameMillis = frameSeconds * 1000;
unsigned long previousMillis = 0;

const point point_zero = point(0, 0);

//DIRECTIONS
const point up = point(0, 1);
const point down = point(0, -1);
const point left = point(-1, 0);
const point right = point(1, 0);

//INPUT STATES
int lastInputDir = 0;
int lastRotateInputDir = 0;
// int leftMove_lastPressTime = 0;
// int rightMove_lastPressTime = 0;
// int leftRotate_lastPressTime = 0;
// int rightRotate_lastPressTime = 0;
// bool leftMoveHeld = false;
// bool rightMoveHeld = false;
// bool leftRotateHeld = false;
// bool rightRotateHeld = false;
// bool resetHeld = false;
// bool pauseHeld = false;
// bool swapHoldHeld = false;
// bool hardDropHeld = false;

//TEMP STATES/VALUES
bool ledDisplayUpdated = false;
unsigned int frameCount = 0;
unsigned int gameOverFrame = 0;
bool softDrop = false;
int waitTillFrame = 0;
int waitToStartFrames = 60;
int clearedLines = 0;
int score = 0;
uint8_t level = 1;
bool topOut = false;
int movementHeldFrames = 0;
int steps = 0;
bool ignoreStepThisFrame = false;
bool hasHeldThisTurn = false;
int infinitySteps;
int softDropScoreAccumulated;
int lastSuccessfullStep = 0;
Trick lastLineClearTrick = noTrick;
TSpinType lastMoveTSpinType = none;
bool paused = false;
uint8_t lineClearAnimation = DISPLAY_BOARD_WIDTH;
uint8_t topOutAnimation = DISPLAY_BOARD_HEIGHT;

point ghostShape[TETROMINO_SIZE] = {point(0, 0), point(0, 0), point(0, 0), point(0, 0)};
CRGB ghostColor = CRGB::Black;

CRGB lineClearColor = CRGB ::White;

int queueIndex = 0;
short tetrominoQueue[] = {0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6};
//char inputData = 0;

#pragma endregion

//* Structs
#pragma region Structs

struct BasicShape
{
    point cells[TETROMINO_SIZE];
    CRGB color;
    ShapeType shapeType;

    BasicShape() : color(CRGB::Black){};

    BasicShape(point cells[TETROMINO_SIZE], CRGB color, ShapeType shapeType) : color(color), shapeType(shapeType)
    {
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            (*this).cells[i] = cells[i];
        }
    }

    int minShapeY()
    {
        int min = INT_MAX;
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            min = min(min, cells[i].y);
        }
        return min;
    }

    int maxShapeY()
    {
        int max = INT_MIN;
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            max = max(max, cells[i].y);
        }
        return max;
    }
};

struct Tetromino
{
    BasicShape shape;
    //point worldOffset;
    point worldPositions[TETROMINO_SIZE];
    point localShape[TETROMINO_SIZE];
    int currentRotationState;
    bool active;
    point shapeSize;

    Tetromino() : shape(BasicShape()), active(false){};

    //Tetromino(point cells[TETROMINO_SIZE], CRGB color) : color(color), active(true)
    Tetromino(BasicShape shape) : active(true)
    {
        (*this).shape = shape;
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            (*this).worldPositions[i] = shape.cells[i];
            (*this).localShape[i] = shape.cells[i];
        }
        shapeSize = getShapeSize(shape.cells);
    }

    TSpinType getTSpinCornerState()
    {
        if (shape.shapeType != T)
            return none;

        //Find center of T block
        //Block centerBlock = null;
        point centerBlock = point(0, 0);
        point centerCell = point(shapeSize.x / 2, shapeSize.y / 2);
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            if (localShape[i].x == centerCell.x && localShape[i].y == centerCell.y)
            {
                centerBlock = worldPositions[i];
                break;
            }
        }

        //Count obstructions in the four corners surrounding the T block
        int frontCorners = 0;
        int backCorners = 0;
        for (int i = 0; i < 4; i++)
        {
            //we can shift the starting array by our currentRotationState in order to keep track of which corners are in front of us
            point corner = tSpinCorners[(i + currentRotationState) % 4];
            //if (!centerBlock.canMove(corner, blocks))
            if (!spaceIsFree(corner, worldPositions))
            {
                if (i < 2)
                    frontCorners++;
                else
                    backCorners++;
            }
        }

        //We need 3 corners total to count a T spin, if two of them are from our front corner, then it is a full t spin
        if (frontCorners + backCorners >= 3)
        {
            if (frontCorners == 2)
                return full;
            else
                return mini;
        }
        else
            return none;
    }

    void resetPosition()
    {
        currentRotationState = 0;
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            worldPositions[i] = shape.cells[i];
            localShape[i] = shape.cells[i];
        }
    }

    bool canMove(point direction)
    {
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            point newPos = worldPositions[i] + direction;
            if (!withinBounds(newPos))
                return false;

            if (!isOccupied(newPos))
                continue;

            bool canIgnoreObstruction = false;
            for (int o = 0; o < TETROMINO_SIZE; o++)
            {
                if (newPos == worldPositions[o])
                {
                    canIgnoreObstruction = true;
                    break;
                }
            }
            if (!canIgnoreObstruction)
                return false;
        }

        return true;
    }

    bool tryRotate(int direction)
    {
        if (shape.shapeType == O)
            return true;

        float rad = -radians(direction * 90);
        float rotSin = sin(rad);
        float rotCos = cos(rad);
        float center = ((float)max(shapeSize.x, shapeSize.y)) / 2;
        int nextRotationState = nmod(currentRotationState + direction, 4);

        point *kickOffsets = getTable(currentRotationState, direction, shape.shapeType == I);
        point rotateVectors[TETROMINO_SIZE] = {
            point(localShape[0].x - center, localShape[0].y - center),
            point(localShape[1].x - center, localShape[1].y - center),
            point(localShape[2].x - center, localShape[2].y - center),
            point(localShape[3].x - center, localShape[3].y - center),
        };

        //Set up rotation vectors
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            float xOffset = (float)localShape[i].x - center;
            float yOffset = (float)localShape[i].y - center;

            rotateVectors[i] = point(
                round(rotCos * xOffset - rotSin * yOffset + center),
                round(rotSin * xOffset - rotCos * yOffset + center));
        }

        //Run safety checks against kick table
        //!May be acting funny? line seems to kick when it doesn't need to
        //? ^^^ I think this is fixed, needs more tests
        int sucessfulOffsetIndex = KICKTABLE_SIZE;
        point determinedKick = point(0, 0);
        for (int o = -1; o < KICKTABLE_SIZE; o++)
        {
            if (o >= 0)
                determinedKick = kickOffsets[o];

            boolean offsetSuccess = true;
            for (int i = 0; i < TETROMINO_SIZE; i++)
            {
                point worldRotation = point(
                    worldPositions[i].x + (rotateVectors[i].x - localShape[i].x) + determinedKick.x,
                    worldPositions[i].y + (rotateVectors[i].y - localShape[i].y) + determinedKick.y);

                if (!spaceIsFree(worldRotation, worldPositions))
                {
                    offsetSuccess = false;
                    break;
                }
            }

            if (offsetSuccess)
            {
                sucessfulOffsetIndex = o;
                break;
            }
        }

        if (sucessfulOffsetIndex > KICKTABLE_SIZE)
            return false;

        //If we made it to this point, the rotation is valid. Begin the actual transformation
        clearPixels();

        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            point worldRotation = point(
                worldPositions[i].x + (rotateVectors[i].x - localShape[i].x) + determinedKick.x,
                worldPositions[i].y + (rotateVectors[i].y - localShape[i].y) + determinedKick.y);

            worldPositions[i] = worldRotation;
            setBoardPixel(worldPositions[i].x, worldPositions[i].y, shape.color);

            localShape[i].x = rotateVectors[i].x;
            localShape[i].y = rotateVectors[i].y;
        }

        currentRotationState = nextRotationState;

        //If the rotation uses the last value on the kick table (1 by 2 blocks), then it is a valid full t spin, otherwise it is a mini
        //https://tetris.wiki/T-Spin
        //https://winternebs.github.io/TETRIS-FAQ/tspin/
        if (sucessfulOffsetIndex == 4)
            lastMoveTSpinType = full;
        else
            lastMoveTSpinType = mini;

        return true;
    }

    bool move(point direction)
    {
        clearPixels();

        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            worldPositions[i] = worldPositions[i] + direction;
            setBoardPixel(worldPositions[i].x, worldPositions[i].y, shape.color);
        }
    }

    void clearPixels()
    {
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            clearBoardPixel(worldPositions[i].x, worldPositions[i].y);
        }
    }

    void placePixels()
    {
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            setBoardPixel(worldPositions[i].x, worldPositions[i].y, shape.color);
        }
    }

    // bool canJump(point cell)
    // {
    //     point center = getCenter(shapeSize);
    //     for (int i = 0; i < TETROMINO_SIZE; i++)
    //     {
    //         point offsetPosition = point(cell.x - center.x + localShape[i].x, cell.y - center.y + localShape[i].y);
    //         if (spaceIsFree(offsetPosition, worldPositions))
    //             return false;
    //     }

    //     return true;
    // }

    // bool jump(point cell)
    // {
    //     point center = getCenter(shapeSize);

    //     clearPixels();

    //     for (int i = 0; i < TETROMINO_SIZE; i++)
    //     {
    //         worldPositions[i] = point(cell.x - center.x + localShape[i].x, cell.y - center.y + localShape[i].y);
    //         setBoardPixel(worldPositions[i].x, worldPositions[i].y, shape.color);
    //     }
    // }

    point getLowest()
    {
        point p = point(0, DISPLAY_BOARD_HEIGHT);

        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            if (worldPositions[i].y < p.y)
                p = worldPositions[i];
        }

        return p;
    }

    point getLeftMost()
    {
        point p = point(DISPLAY_BOARD_WIDTH, 0);

        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            if (worldPositions[i].x < p.x)
                p = worldPositions[i];
        }

        return p;
    }

    point getRightMost()
    {
        point p = point(0, 0);

        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            if (worldPositions[i].x > p.x)
                p = worldPositions[i];
        }

        return p;
    }

    int getDistanceToGround()
    {
        int distance = 0;
        do
        {
            distance--;
        } while (canMove(point(0, distance)));

        return distance;
    }

    bool jumpToSpawnPosition()
    {
        point spawnPosition = point(DISPLAY_BOARD_WIDTH / 2, DISPLAY_BOARD_HEIGHT - (shapeSize.y + 1));

        bool success = true;
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            worldPositions[i] = point(shape.cells[i].x + spawnPosition.x - ceil(((float)shapeSize.x + 1) / 2), spawnPosition.y + shape.cells[i].y);

            if (isOccupied(worldPositions[i]) || !withinBounds(worldPositions[i]))
                success = false;

            setBoardPixel(worldPositions[i].x, worldPositions[i].y, shape.color);
        }

        return success;
    }

    bool trySpawn()
    {
        if (jumpToSpawnPosition())
        {
            if (canMove(down))
                move(down);
        }
        else //*TOP OUT. LOSE CONDITION
        {
            topOutGameOver();
            return false;
        }

        return true;
    }
};

struct AI_Move
{
    float moveScore = INT_MIN;
    int rotationState = 0;
    int leftMostCell = 0;

    AI_Move(float move_Score, int move_Rotation, int move_leftMostCell) : moveScore(move_Score), rotationState(move_Rotation), leftMostCell(move_leftMostCell){};
};

#pragma endregion

//* Struct Declarations and Variables
#pragma region Struct Declarations and Variables

void resetAndRotateTestBlock(Tetromino &testTetromino, int rotation);
AI_Move updateBestMove(Tetromino &testTetromino, int holdQueueIndex);
Tetromino currentTetromino;
BasicShape heldTetromino;
const Tetromino allTetrominoTypes[] = {J_SHAPE, L_SHAPE, I_SHAPE, S_SHAPE, Z_SHAPE, O_SHAPE, T_SHAPE};

#pragma endregion

//* KickTables
#pragma region KickTables

point empty_table[KICKTABLE_SIZE] = {point(0, 0), point(0, 0), point(0, 0), point(0, 0)}; //0 >> 1

point standard_table_0_1[KICKTABLE_SIZE] = {point(-1, 0), point(-1, 1), point(0, -2), point(-1, -2)}; //0 >> 1
point standard_table_1_0[KICKTABLE_SIZE] = {point(1, 0), point(1, -1), point(0, 2), point(1, 2)};     //1 >> 0
point standard_table_1_2[KICKTABLE_SIZE] = {point(1, 0), point(1, -1), point(0, 2), point(1, 2)};     //1 >> 2
point standard_table_2_1[KICKTABLE_SIZE] = {point(-1, 0), point(-1, 1), point(0, -2), point(-1, -2)}; //2 >> 1
point standard_table_2_3[KICKTABLE_SIZE] = {point(1, 0), point(1, 1), point(0, -2), point(1, -2)};    //2 >> 3
point standard_table_3_2[KICKTABLE_SIZE] = {point(-1, 0), point(-1, -1), point(0, 2), point(-1, 2)};  //3 >> 2
point standard_table_3_0[KICKTABLE_SIZE] = {point(-1, 0), point(-1, -1), point(0, 2), point(-1, 2)};  //3 >> 0
point standard_table_0_3[KICKTABLE_SIZE] = {point(1, 0), point(1, 1), point(0, -2), point(1, -2)};    //0 >> 3

//! I shape sometimes has trouble with wallkicks
point iShape_table_0_1[KICKTABLE_SIZE] = {point(-2, 0), point(1, 0), point(-2, -1), point(1, 2)}; //0 >> 1
point iShape_table_1_0[KICKTABLE_SIZE] = {point(2, 0), point(-1, 0), point(2, 1), point(-1, -2)}; //1 >> 0
point iShape_table_1_2[KICKTABLE_SIZE] = {point(-1, 0), point(2, 0), point(-1, 2), point(2, -1)}; //1 >> 2
point iShape_table_2_1[KICKTABLE_SIZE] = {point(1, 0), point(-2, 0), point(1, -2), point(-2, 1)}; //2 >> 1
point iShape_table_2_3[KICKTABLE_SIZE] = {point(2, 0), point(-1, 0), point(2, 1), point(-1, -2)}; //2 >> 3
point iShape_table_3_2[KICKTABLE_SIZE] = {point(-2, 0), point(1, 0), point(-2, -1), point(1, 2)}; //3 >> 2
point iShape_table_3_0[KICKTABLE_SIZE] = {point(1, 0), point(-2, 0), point(1, -2), point(-2, 1)}; //3 >> 0
point iShape_table_0_3[KICKTABLE_SIZE] = {point(-1, 0), point(2, 0), point(-1, 2), point(2, -1)}; //0 >> 3

point *getTable(int currentState, int direction, bool isIShape)
{
    switch (currentState)
    {
    case 0:
        if (direction > 0)
            return isIShape ? iShape_table_0_1 : standard_table_0_1;
        else
            return isIShape ? iShape_table_0_3 : standard_table_0_3;
        break;

    case 1:
        if (direction > 0)
            return isIShape ? iShape_table_1_2 : standard_table_1_2;
        else
            return isIShape ? iShape_table_1_0 : standard_table_1_0;
        break;

    case 2:
        if (direction > 0)
            return isIShape ? iShape_table_2_3 : standard_table_2_3;
        else
            return isIShape ? iShape_table_2_1 : standard_table_2_1;
        break;

    case 3:
        if (direction > 0)
            return isIShape ? iShape_table_3_0 : standard_table_3_0;
        else
            return isIShape ? iShape_table_3_2 : standard_table_3_2;
        break;
    }

    return empty_table;
}

#pragma endregion

//* LED
#pragma region LED

void bootSequenceLED()
{
    int snakeDelay = 1; //frameMillis / 4;
    int wipeDelay = frameMillis;

    for (int i = 0; i < DISPLAY_BOARD_HEIGHT; i++)
    {
        setBoardPixel(0, i, CRGB ::White);
        setBoardPixel(9, i, CRGB ::White);
        FastLED.show();
        delay(wipeDelay * 4);
    }
    //snakeLED(5, snakeDelay, CRGB::White);
    wipeHorizontalLED(wipeDelay, CRGB::White);
    wipeVerticalLED(wipeDelay, CRGB::White);

    // snakeLED(5, snakeDelay, CRGB::Red);
    // wipeHorizontalLED(wipeDelay, CRGB::Red);
    // wipeVerticalLED(wipeDelay, CRGB::Red);

    // snakeLED(5, snakeDelay, CRGB::Green);
    // wipeHorizontalLED(wipeDelay, CRGB::Green);
    // wipeVerticalLED(wipeDelay, CRGB::Green);

    // snakeLED(5, snakeDelay, CRGB::Blue);
    // wipeHorizontalLED(wipeDelay, CRGB::Blue);
    // wipeVerticalLED(wipeDelay, CRGB::Blue);
}

int ledIndexVertical(int x, int y, int yLength, bool inverse, int offset)
{
    return (inverse - x & 1) * (yLength - 2 * y - 1) + yLength * x + y + offset;
}

int ledIndexHorizontal(int x, int y, int xLength, bool inverse, int offset)
{
    return (inverse - y & 1) * (xLength - 2 * x - 1) + (xLength * y + x) + offset;
}

int getBoardIndex(int x, int y)
{
    return ledIndexVertical(x, y, DISPLAY_BOARD_HEIGHT, true, DISPLAY_BOARD_OFFSET_INDEX);
}

void setBoardPixel(int x, int y, CRGB setColor)
{
    leds[getBoardIndex(x, y)] = setColor;
    ledDisplayUpdated = true;
}

void clearBoardPixel(int x, int y)
{
    setBoardPixel(x, y, CRGB::Black);
}

int getQueueIndex(int x, int y)
{
    return ledIndexVertical(x, y, DISPLAY_QUEUE_HEIGHT, true, DISPLAY_QUEUE_OFFSET_INDEX);
}

void setQueuePixel(int x, int y, CRGB setColor)
{
    leds[getQueueIndex(x, y)] = setColor;
    ledDisplayUpdated = true;
}

void clearQueuePixel(int x, int y)
{
    setQueuePixel(x, y, CRGB::Black);
}

int getHoldIndex(int x, int y)
{
    return ledIndexHorizontal(x, y, DISPLAY_HOLD_WIDTH, true, DISPLAY_HOLD_OFFSET_INDEX);
}

void setHoldPixel(int x, int y, CRGB setColor)
{
    leds[getHoldIndex(x, y)] = setColor;
    ledDisplayUpdated = true;
}

void clearHoldPixel(int x, int y)
{
    setHoldPixel(x, y, CRGB::Black);
}

void snakeLED(int snakeLength, int delayMillis, CRGB setColor)
{
    for (int i = 0; i < NUM_LEDS + delayMillis; i++)
    {
        leds[nmod(i - snakeLength, NUM_LEDS)] = CRGB::Black;
        if (i < NUM_LEDS)
            leds[i] = setColor;

        delay(delayMillis);
        FastLED.show();
    }
}

void wipeHorizontalLED(int delayMillis, CRGB setColor)
{
    for (int x = 0; x <= displayMainWidth; x++)
    {
        for (int y = 0; y < DISPLAY_BOARD_HEIGHT; y++)
        {
            setFullScreenLED(nmod(x - 1, displayMainWidth), y, CRGB::Black);
            setFullScreenLED(x, y, setColor);
        }

        FastLED.show();
        delay(delayMillis);
    }
}

void wipeVerticalLED(int delayMillis, CRGB setColor)
{
    for (int y = DISPLAY_BOARD_HEIGHT - 1; y >= -1; y--)
    {
        for (int x = 0; x < displayMainWidth; x++)
        {
            setFullScreenLED(x, nmod(y + 1, DISPLAY_BOARD_HEIGHT), CRGB::Black);
            setFullScreenLED(x, y, setColor);
        }

        FastLED.show();
        delay(delayMillis);
    }
}

void setFullScreenLED(int x, int y, CRGB setColor)
{
    if (x < 0 || y < 0)
        return;

    int index = -1;

    if (x < DISPLAY_HOLD_WIDTH)
    {
        int yOffset = DISPLAY_BOARD_HEIGHT - DISPLAY_HOLD_HEIGHT;
        if (y >= yOffset && y < DISPLAY_BOARD_HEIGHT)
            index = ledIndexHorizontal(x, y - yOffset, DISPLAY_HOLD_WIDTH, true, DISPLAY_HOLD_OFFSET_INDEX);
    }
    else if (x < DISPLAY_HOLD_WIDTH + DISPLAY_BOARD_WIDTH)
    {
        if (y < DISPLAY_BOARD_HEIGHT)
            index = ledIndexVertical(x - DISPLAY_HOLD_WIDTH, y, DISPLAY_BOARD_HEIGHT, true, DISPLAY_BOARD_OFFSET_INDEX);
    }
    else if (x < displayMainWidth)
    {
        int yOffset = DISPLAY_BOARD_HEIGHT - DISPLAY_QUEUE_HEIGHT;
        if (y >= yOffset && y < DISPLAY_BOARD_HEIGHT)
            index = ledIndexVertical(x - (DISPLAY_HOLD_WIDTH + DISPLAY_BOARD_WIDTH), y - yOffset, DISPLAY_QUEUE_HEIGHT, true, DISPLAY_QUEUE_OFFSET_INDEX);
    }

    if (index >= 0 && index < NUM_LEDS)
        leds[index] = setColor;
}

#pragma endregion

//* Helper Functions
#pragma region Helper Functions

bool withinBounds(point cell)
{
    return (cell.x >= 0 && cell.y >= 0 && cell.x < DISPLAY_BOARD_WIDTH && cell.y < DISPLAY_BOARD_HEIGHT);
}

bool isOccupied(point cell)
{
    int index = getBoardIndex(cell.x, cell.y);
    return (leds[index].r != 0 || leds[index].g != 0 || leds[index].b != 0);
}

int clearedRows[TETROMINO_SIZE] = {-1, -1, -1, -1};
int countClearedRows()
{
    for (int i = 0; i < TETROMINO_SIZE; i++)
    {
        if (clearedRows[i] < 0)
            return i;
    }

    return TETROMINO_SIZE;
}

point getCenter(point size)
{
    return point(ceil((float)(size.x + 1) / 2), ceil((float)size.y / 2));
}

point getShapeSize(point cells[TETROMINO_SIZE])
{
    int maxX = 0;
    int maxY = 0;
    for (int i = 0; i < TETROMINO_SIZE; i++)
    {
        point cell = cells[i];

        maxX = max(maxX, cell.x);
        maxY = max(maxY, cell.y);
    }

    return point(maxX, maxY);
}

bool spaceIsFree(point cell, point ignoredCells[TETROMINO_SIZE])
{
    if (!withinBounds(cell))
        return false;

    if (isOccupied(cell))
    {
        for (int i = 0; i < TETROMINO_SIZE; i++)
        {
            if (cell == ignoredCells[i])
                return true;
        }

        return false;
    }
    else
        return true;
}

bool hasLinesToClear()
{
    return countClearedRows() > 0;
}

// Modulo that processes negative values. Useful for arrays or integers that have a minimum value of 0.
static int nmod(float x, float m)
{
    return (int)(x - m * floor(x / m));
}

float pingpong(float start, float stop, float dist)
{
    float d = stop - start;
    if ((int)(abs(dist) / d) % 2 == 0)
    {
        return start + fmod(abs(dist), d);
    }
    else
    {
        return stop - fmod(abs(dist), d);
    }
}

bool hasHeldTetromino()
{
    return heldTetromino.color.r != 0 || heldTetromino.color.g != 0 || heldTetromino.color.b != 0;
}

float currentGravityFrames()
{
    float seconds = pow((0.8f - ((level - 1) * 0.007)), level - 1);

    return seconds * FRAMERATE;
}

#pragma endregion

//* Arduino Methods
#pragma region Arduino Methods

void setup()
{
    Serial.begin(9600);
    psx.begin();
    randomSeed(analogRead(0) + analogRead(1));

    delay(3000); // power-up safety delay
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);

    bootSequenceLED();

#ifdef _AI_TRAINING_MODE
    establishSerialContact();
#else
    resetGame();
#endif
}

void loop()
{
#ifdef _AI_TRAINING_MODE
    AI_Training_serialUpdate();
    if (ai_waitForNewWeights)
    {
        if (ai_gameEndFrame > 0 && (ai_gameEndFrame + FRAMERATE * 3) < frameCount)
        {
            if (frameCount % FRAMERATE == 0)
            {
                builtInLED_State = !builtInLED_State;
                digitalWrite(LED_BUILTIN, builtInLED_State);
                Serial.print("WAITING\n");
            }
        }
        return;
    }
#endif
    inputUpdate();

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis < frameMillis)
        return;
    previousMillis = currentMillis;

    if (topOut)
    {
#ifdef _AI_TRAINING_MODE
        completeTrainingRound();
        return;
#endif
        if (frameCount >= gameOverFrame + restartAfterGameoverAsAI_frames)
        {
            controlledByAI = true;
            resetGame();
        }
    }

    animateLineClears();
    animateTopOut();

    for (int i = stepCheck(); i > 0; i--)
    {
        if (currentTetromino.active)
            fallUpdate();
        else
        {
            clearedLineCleanup();
            spawnNewTetromino();
        }
    }

    //*END OF FRAME BUSINESS
    if (!controlledByAI)
        renderGhost();
    if (ledDisplayUpdated)
    {
        FastLED.show();
        ledDisplayUpdated = false;
    }
    if (!controlledByAI)
        clearGhost();

    frameCount++;
}

#pragma endregion

//* AI Training
#pragma region AI Training
#ifdef _AI_TRAINING_MODE

void completeTrainingRound()
{
    currentTetromino.clearPixels();
    currentTetromino.active = false;
    ai_waitForNewWeights = true;
    ai_gameEndFrame = frameCount;
    Serial.println(_SERIAL_GAME_END);
}

void establishSerialContact()
{
    while (Serial.available() <= 0)
    {
        Serial.println(_SERIAL_CONTACT); // send a capital A
        digitalWrite(LED_BUILTIN, true);
        delay(300);
    }
}

void writeSerialWeights(float *weights)
{
    for (int i = 0; i < 4; i++)
    {
        Serial.print(weights[i]);
        if (i == 3)
            Serial.print('\n');
        else
            Serial.print(',');
    }
}

void resetWithSerialDataWeights()
{
    weight_aggregateHeight = Serial.readStringUntil(',').toFloat();
    weight_completeLines = Serial.readStringUntil(',').toFloat();
    weight_holes = Serial.readStringUntil(',').toFloat();
    weight_bumpiness = Serial.readStringUntil('\n').toFloat();
    Serial.println(_SERIAL_GAME_START);
    Serial.print("F ");
    Serial.println(freeMemory());
    resetGame();
}

void AI_Training_serialUpdate()
{
    if (Serial.available() > 0)
    { // If data is available to read,
        // val = Serial.readStringUntil('\n'); // read it and store it in val
        // val.trim();
        char initialChar = Serial.read();

        if (initialChar == _SERIAL_WEIGHT_VALUE) //if we get a weight signal
        {
            resetWithSerialDataWeights();
        }
        else
        {
            Serial.readStringUntil('\n');
            Serial.println("RESPONSE");
        }
    }
    // else
    // {
    //     Serial.println('P'); //send back a polling signal
    //     delay(50);
    // }
}

#endif
#pragma endregion

//* Game Management
#pragma region Game Management

void resetGame()
{
    currentTetromino.clearPixels();
    currentTetromino.active = false;
    for (int x = 0; x < DISPLAY_BOARD_WIDTH; x++)
    {
        for (int y = 0; y < DISPLAY_BOARD_HEIGHT; y++)
        {
            clearBoardPixel(x, y);
        }
    }

    paused = false;
    clearedLines = 0;
    level = 1;
    score = 0;
    topOut = false;
    frameCount = 0;
    gameOverFrame = 0;
    softDrop = false;
    total_linesCleared = 0;
    total_tetrominos = 0;

    queueIndex = 7;
    updateTetrominoPool();
    queueIndex = 0;
    updateTetrominoPool();

    tetrominoStartState();
    heldTetromino.color = CRGB::Black;
    hasHeldThisTurn = false;
    redrawHold();
    lastLineClearTrick = noTrick;

#ifdef _AI_TRAINING_MODE
    waitTillFrame = frameCount + 1;
    ai_waitForNewWeights = false;
    ai_gameEndFrame = 0;
#else _AI_TRAINING_MODE
    waitTillFrame = frameCount + waitToStartFrames;
    spawnNewTetromino();
    Serial.print("LEVEL ");
    Serial.println(level);
#endif
}

void topOutGameOver()
{
    currentTetromino.active = false;
    topOut = true;
    topOutAnimation = 0;
    gameOverFrame = frameCount;
}

void levelUpCheck()
{
    int oldLevel = level;
    level = constrain(floor(clearedLines / LINESPERLEVEL) + 1, 1, MAXLEVEL);

    if (oldLevel < level)
        levelUpEffects();
}

void updateTetrominoPool()
{
    //Fisher-Yates shuffle
    //https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
    int addition = queueIndex < TETROMINO_COUNT ? TETROMINO_COUNT : 0;
    for (int i = TETROMINO_COUNT + addition - 1; i > addition; i--)
    {
        int index = random(TETROMINO_COUNT) + addition;
        // Simple swap
        int a = tetrominoQueue[index];
        tetrominoQueue[index] = tetrominoQueue[i];
        tetrominoQueue[i] = a;
    }
}

#pragma endregion

//* Timing, End Tetromino and Spawning
#pragma region Timing

//returns the amount of steps to occur this frame
int stepCheck()
{
    if (paused || topOut)
        return 0;

    if (frameCount < waitTillFrame)
        return 0;

    float stepRate = currentGravityFrames(); //default fall rate

#ifdef _AI_TRAINING_MODE
    // if (hasLinesToClear())
    //     stepRate = 30;
    // else
    stepRate = 1;
#else
    if (hasLinesToClear())
        stepRate = LINECLEAREDRATE; //wait for line to clear before continuing
    else if (currentTetromino.active)
    {
        if (!currentTetromino.canMove(down))
            stepRate = LOCKOUTRATE; //lock out time (half a second)
        else if (softDrop && stepRate > 0)
            stepRate = round((float)stepRate / SOFTDROPMODIFIER); //fast fall modifier of current rate
    }
    else // set the step rate to zero so we can create a new tetromino right away
        stepRate = 0;
#endif

    int stepsThisFrame = 0;
    if (!ignoreStepThisFrame)
    {
        int roundedStepRate = round(stepRate);
        bool instantLand = roundedStepRate <= 0;
        if (instantLand && currentTetromino.active) //if stepRate is less than 0 (and there is a tetromino), jump to the bottom of the playspace
            stepsThisFrame = abs(currentTetromino.getDistanceToGround()) - 1;
        else if (instantLand || steps - lastSuccessfullStep >= roundedStepRate) //steps % round(roundedStepRate) == 0)//move one step this frame if we are at the point to move
            stepsThisFrame = 1;
    }

    if (stepsThisFrame > 0)
        lastSuccessfullStep = steps;

    steps++;
    ignoreStepThisFrame = false;

    return stepsThisFrame;
}

void resetSteps()
{
    lastSuccessfullStep = 0;
    steps = 0;
}

void endCurrentTetromino()
{
    int linesCleared = countAndRegisterClearedLines();
    if (linesCleared > 0)
    {
        paintClearedLines();
    }

    ignoreStepThisFrame = linesCleared > 0;

    total_linesCleared += linesCleared;
#ifdef _AI_TRAINING_MODE
    Serial.println(total_linesCleared);
#endif
    trickCalculation(linesCleared);

    if (currentTetromino.getLowest().y >= VANISHINGLINE)
        topOutGameOver();

    currentTetromino.active = false;
    hasHeldThisTurn = false;

    tetrominoStartState();
#if defined _DEBUG && !defined _AI_TRAINING_MODE
    debugAIHeuristics();
#endif
}

void spawnNewTetromino()
{
#ifdef _AI_TRAINING_MODE
    if (total_tetrominos == _AI_TRAINING_MAX_TETROMINOS)
    {
        Serial.println("TOTAL TETRONIMOS REACHED");
        completeTrainingRound();
    }

#endif

    levelUpCheck();
    currentTetromino = allTetrominoTypes[tetrominoQueue[queueIndex]];
    if (currentTetromino.trySpawn())
    {
        Serial.print(_SERIAL_TETROMINOS_VALUE);
        Serial.print(' ');
        Serial.println(total_tetrominos);
        total_tetrominos++;
    }

    queueIndex = (queueIndex + 1) % (TETROMINO_COUNT * 2);
    if (queueIndex % TETROMINO_COUNT == 0)
        updateTetrominoPool();

    redrawQueue();

    if (controlledByAI)
        onAISpawn();
}

void tetrominoStartState()
{
    softDropScoreAccumulated = 0;
    infinitySteps = MAXINFINITYSTEPS;
    lastMoveTSpinType = none;
    resetSteps();
}

void swapHoldTetromino()
{
    if (paused || hasHeldThisTurn)
        return;

    currentTetromino.clearPixels();
    currentTetromino.resetPosition();

    BasicShape buffer = currentTetromino.shape;

    if (hasHeldTetromino())
    {
        currentTetromino = heldTetromino;
        currentTetromino.trySpawn();
    }
    else
        spawnNewTetromino();

    heldTetromino = buffer;
    tetrominoStartState();

    hasHeldThisTurn = true;
    redrawHold();
}

#pragma endregion

//* Input, Movement
#pragma region Input, Movement

byte psxButtonToIndex(PsxButtons psxButtons)
{
    byte i;

    for (i = 0; i < PSX_BUTTONS_NO; ++i)
    {
        if (psxButtons & 0x01)
        {
            break;
        }

        psxButtons >>= 1U;
    }

    return i;
}

void inputUpdate()
{
    processPlayerInput();

    if (paused || !currentTetromino.active)
        return;

    //*AI here should override current player input from this point forwards
    if (controlledByAI)
    {
        updateAI();
    }
    else
    {
        if (psx.buttonJustPressed(PSB_L1) ||
            psx.buttonJustPressed(PSB_L2) ||
            psx.buttonJustPressed(PSB_R1) ||
            psx.buttonJustPressed(PSB_R2)) //(inputData == CONTROLS_HOLD)
            swapHoldTetromino();
        else if (psx.buttonJustPressed(PSB_PAD_UP)) //(inputData == CONTROLS_HARDDROP)
            hardDrop();

        if (psx.buttonPressed(PSB_PAD_DOWN))
            softDrop = true;
        else
            softDrop = false;

        bool successfulMove = moveInput(playerDirectionInput());
        bool successfulRotation = rotateInput(playerRotationInput());

        finishInput(successfulMove || successfulRotation);
    }
}

void finishInput(bool successfulMovement)
{
    if (successfulMovement && infinitySteps > 0 && !currentTetromino.canMove(down))
    {
        resetSteps();
        ignoreStepThisFrame = true;
        infinitySteps--;
    }
}

void processPlayerInput()
{
    psx.read();

    if (psx.buttonJustPressed(PSB_START)) //(inputData == CONTROLS_PAUSE)
        paused = !paused;
#ifndef _AI_TRAINING_MODE
    else if (psx.buttonJustPressed(PSB_SELECT)) //(inputData == CONTROLS_RESET)
    {
        controlledByAI = false;
        resetGame();
    }
#endif
}

int playerDirectionInput()
{
    int dir = 0;
    if (psx.buttonPressed(PSB_PAD_LEFT))
        dir = -1;
    else if (psx.buttonPressed(PSB_PAD_RIGHT))
        dir = 1;

    return dir;
}

int playerRotationInput()
{
    int dir = 0;
    if (psx.buttonPressed(PSB_CROSS) || psx.buttonPressed(PSB_SQUARE))
        dir = -1;
    else if (psx.buttonPressed(PSB_CIRCLE) || psx.buttonPressed(PSB_TRIANGLE))
        dir = 1;

    return dir;
}

bool moveInput(int dir)
{
    bool success = false;

    if (dir != 0)
    {
        if (dir != lastInputDir)
            movementHeldFrames = 0;

        if (controlledByAI || (movementHeldFrames == 0 || (movementHeldFrames >= DELAYEDAUTOSHIFT && (movementHeldFrames - DELAYEDAUTOSHIFT) % AUTOREPEATRATE == 0)))
        {
            point moveDir = point(dir, 0);
            if (currentTetromino.canMove(moveDir))
            {
                currentTetromino.move(moveDir);
                lastMoveTSpinType = none;
                success = true;
            }
        }

        movementHeldFrames++;
    }
    else
        movementHeldFrames = 0;

    lastInputDir = dir;

    return success;
}

boolean rotateInput(int dir)
{
    boolean success;
    if (dir != 0 && (controlledByAI || dir != lastRotateInputDir))
        success = currentTetromino.tryRotate(dir);
    else
        success = false;

    lastRotateInputDir = dir;

    return success;
}

void jump(point start, point end)
{
    int index = getBoardIndex(start.x, start.y);
    setBoardPixel(end.x, end.y, leds[index]);
    leds[index] = CRGB::Black;
}

void hardDrop()
{
    if (paused)
        return;

    int points = 0;
    while (currentTetromino.canMove(down))
    {
        points += SSPCD_HARDDROP;
        currentTetromino.move(down);
        lastMoveTSpinType = none;
    }

    if (!controlledByAI)
        score += min(points, SSPCDMAX_HARDDROP);
    endCurrentTetromino();
}

void fallUpdate()
{
    if (currentTetromino.canMove(down))
    {
        currentTetromino.move(down);
        lastMoveTSpinType = none;

        if (!controlledByAI)
            softDropScoring();
    }
    else
        endCurrentTetromino();
}

#pragma endregion

//* Rendering, Animation
#pragma region Rendering, Animation

CRGB desaturate(CRGB source, float intensity, float brightness)
{
    float l = 0.3 * source.r + 0.59 * source.g + 0.11 * source.b;
    source.r = (source.r + intensity * (l - source.r)) * brightness;
    source.g = (source.g + intensity * (l - source.g)) * brightness;
    source.b = (source.b + intensity * (l - source.b)) * brightness;
    return source;
}

void levelUpEffects()
{
#if !defined _AI_TRAINING_MODE
    Serial.print("LEVEL ");
    Serial.println(level);
#endif
    //Level up effects here
}

void trickDisplay(Trick currentTrick, int points, bool backToBack)
{
    //*Serial print values
#if !defined _AI_TRAINING_MODE
    if (backToBack)
        Serial.print("BACK TO BACK ");

    //*Doesn't let us print the full string? at least with Serial.print, so we have to do it letter by letter
    int len = strlen(trick_strings[currentTrick]);
    for (int i = 0; i < len; i++)
    {
        Serial.print(trick_strings[currentTrick][i]);
    }

    Serial.print(": ");
    Serial.print(points);
    Serial.print(" pts\n");
#endif
}

void redrawQueue()
{
    //* Clear the queue
    for (int y = 0; y < DISPLAY_QUEUE_HEIGHT; y++)
    {
        for (int x = 0; x < DISPLAY_QUEUE_WIDTH; x++)
        {
            setQueuePixel(x, y, CRGB::Black);
        }
    }

    int y = DISPLAY_QUEUE_HEIGHT - 1;

    for (int i = 0; i < QUEUESIZE; i++, y -= 2)
    {
        Tetromino queuedTetromino = allTetrominoTypes[tetrominoQueue[(queueIndex + i) % (TETROMINO_COUNT * 2)]];

        int minY = queuedTetromino.shape.minShapeY();
        y = y - (queuedTetromino.shape.maxShapeY() - minY);

        for (int c = 0; c < TETROMINO_SIZE; c++)
        {
            setQueuePixel(queuedTetromino.shape.cells[c].x, y + (queuedTetromino.shape.cells[c].y - minY), queuedTetromino.shape.color);
        }
    }
}

void redrawHold()
{
    //* Clear hold display
    for (int y = 0; y < DISPLAY_HOLD_HEIGHT; y++)
    {
        for (int x = 0; x < DISPLAY_HOLD_WIDTH; x++)
        {
            setHoldPixel(x, y, CRGB::Black);
        }
    }

    if (!hasHeldTetromino())
        return;

    int minY = heldTetromino.minShapeY();

    for (int i = 0; i < TETROMINO_SIZE; i++)
    {
        setHoldPixel(heldTetromino.cells[i].x, heldTetromino.cells[i].y - minY, heldTetromino.color);
    }
}

void clearGhost()
{
    for (int i = 0; i < TETROMINO_SIZE; i++)
    {
        int index = getBoardIndex(ghostShape[i].x, ghostShape[i].y);
        if (leds[index] == ghostColor)
            leds[index] = CRGB::Black;
    }
}

void renderGhost()
{
    if (!currentTetromino.active)
        return;

    int yPivot = -1;
    while (currentTetromino.canMove(point(0, yPivot)))
        yPivot--;

    yPivot++;

    float fade = pingpong(GHOSTFADE_MIN, GHOSTFADE_MAX, (float)frameCount * GHOSTFADE_SPEED);
    ghostColor = desaturate(currentTetromino.shape.color, GHOSTFADE_DESATURATION, fade);

    for (int i = 0; i < TETROMINO_SIZE; i++)
    {
        ghostShape[i] = point(currentTetromino.worldPositions[i].x, currentTetromino.worldPositions[i].y + yPivot);
        if (!isOccupied(ghostShape[i]))
        {
            setBoardPixel(ghostShape[i].x, ghostShape[i].y, ghostColor);
        }
    }
}

void animateLineClears()
{
    if (lineClearAnimation >= DISPLAY_BOARD_WIDTH)
        return;

    int clearUpTo = min(lineClearAnimation + 2, DISPLAY_BOARD_WIDTH - 1);

    for (int i = countClearedRows() - 1; i >= 0; i--)
    {
        //clear blocks in row
        int rowY = clearedRows[i];
        for (int x = lineClearAnimation; x < clearUpTo; x++)
        {
            int clearX = i % 2 == 0 ? x : (DISPLAY_BOARD_WIDTH - 1 - x);

            setBoardPixel(clearX, rowY, lineClearColor); //clearBoardPixel(clearX, rowY);
        }
    }

    lineClearAnimation = clearUpTo;
}

void animateTopOut()
{
    if (topOutAnimation >= DISPLAY_BOARD_HEIGHT || frameCount % 2 == 0)
        return;

    for (int x = 0; x < DISPLAY_BOARD_WIDTH; x++)
    {
        CRGB color = desaturate(leds[getBoardIndex(x, topOutAnimation)], TOPOUT_DESATURATION, TOPOUT_BRIGHTNESS);
        setBoardPixel(x, topOutAnimation, color);
    }

    topOutAnimation++;
}

#pragma endregion

//* Line Clearing
#pragma region Line Clearing

int countAndRegisterClearedLines()
{
    int clearedRowCount = 0;
    for (int y = 0; y < DISPLAY_BOARD_HEIGHT; y++)
    {
        if (isCompleteLine(y))
        {
            clearedRows[clearedRowCount] = y;
            clearedRowCount++;
        }
    }

    for (int i = clearedRowCount; i < TETROMINO_SIZE; i++)
    {
        clearedRows[i] = -1;
    }

    return clearedRowCount;
}

void paintClearedLines()
{
    lineClearAnimation = 0;
    for (int i = countClearedRows() - 1; i >= 0; i--, clearedLines++)
    {
        int rowY = clearedRows[i];
        //paint the blocks with a darker version of their color
        for (int x = 0; x < DISPLAY_BOARD_WIDTH; x++)
        {
            CRGB color = leds[getBoardIndex(x, rowY)];
            color.r *= LINECLEAR_FADE;
            color.g *= LINECLEAR_FADE;
            color.b *= LINECLEAR_FADE;
            setBoardPixel(x, rowY, color);
        }
    }
}

void clearedLineCleanup()
{
    if (!hasLinesToClear())
        return;

    for (int i = countClearedRows() - 1; i >= 0; i--, clearedLines++)
    {
        //clear blocks in row
        int rowY = clearedRows[i];
        for (int x = 0; x < DISPLAY_BOARD_WIDTH; x++)
        {
            clearBoardPixel(x, rowY);
        }

        //Move all blocks about
        for (int y = rowY + 1; y < DISPLAY_BOARD_HEIGHT; y++)
        {
            for (int x = 0; x < DISPLAY_BOARD_WIDTH; x++)
            {
                CRGB color = leds[getBoardIndex(x, y)];
                clearBoardPixel(x, y);
                setBoardPixel(x, y - 1, color);
            }
        }
    }

    for (int i = 0; i < TETROMINO_SIZE; i++)
    {
        clearedRows[i] = -1;
    }
}

#pragma endregion

//* Scoring, Tricks, TSpins
#pragma region Scoring, Tricks, TSpins

void softDropScoring()
{
    if (softDrop)
    {
        softDropScoreAccumulated += SSPCD_SOFTDROP;
        if (softDropScoreAccumulated <= SSPCDMAX_SOFTDROP)
            score += SSPCD_SOFTDROP; //! Does this make sense?
    }
}

void processTrick(int linesCleared, int baseScore, Trick currentTrick)
{
    uint8_t trickVal = static_cast<Trick>(currentTrick);
    bool backToBack = trickVal <= trickBackToBackCap && lastLineClearTrick == currentTrick;
    int points = floor(baseScore * (float)(backToBack ? BACKTOBACK_MOD : 1));
    if (!controlledByAI)
        score += points;

    //Only change/break a back-to-back if lines have been cleared
    if (linesCleared > 0)
        lastLineClearTrick = currentTrick;

    trickDisplay(currentTrick, points, backToBack);
}

void trickCalculation(int linesCleared)
{
    TSpinType tSpinType = tSpinCheck();

    switch (linesCleared)
    {
    case 0:
        if (tSpinType == mini)
            processTrick(linesCleared, SSPL_MINITSPIN, miniTSpin);
        else if (tSpinType == full)
            processTrick(linesCleared, SSPL_TSPIN, tSpin);

        break;

    case 1:
        if (tSpinType == mini)
            processTrick(linesCleared, SSPL_MINITSPIN_SINGLE, miniTSpin_single);
        else if (tSpinType == full)
            processTrick(linesCleared, SSPL_TSPIN_SINGLE, tSpin_single);
        else
            processTrick(linesCleared, SSPL_SINGLE, singleLine);

        break;

    case 2:
        if (tSpinType == mini)
            processTrick(linesCleared, SSPL_MINITSPIN_DOUBLE, miniTSpin_double);
        else if (tSpinType == full)
            processTrick(linesCleared, SSPL_TSPIN_DOUBLE, tSpin_double);
        else
            processTrick(linesCleared, SSPL_DOUBLE, doubleLine);

        break;

    case 3:
        if (tSpinType == full)
            processTrick(linesCleared, SSPL_TSPIN_TRIPLE, tSpin_triple);
        else
            processTrick(linesCleared, SSPL_TRIPLE, tripleLine);

        break;

    case 4:
        processTrick(linesCleared, SSPL_TETRIS, tetris);
        break;
    }
}

//https://tetris.wiki/T-Spin
TSpinType tSpinCheck()
{
    //The last maneuver of the T tetrimino must be a rotation.
    if (lastMoveTSpinType == none)
        return none;

    //If there are two minoes in the front corners of the 3 by 3 square occupied by the T (the front corners are ones next to the sticking out mino of the T) and at least one mino in the two other corners (to the back), it is a "proper" T-spin.
    TSpinType tSpinState = currentTetromino.getTSpinCornerState();

    //Otherwise, if there is only one mino in two front corners and two minoes to the back corners, it is a Mini T-spin.
    //However, if the last rotation that kicked the T moves its center 1 by 2 blocks (the last rotation offset of SRS), it is still a proper T-spin.
    if (tSpinState == mini)
        tSpinState = lastMoveTSpinType;

    return tSpinState;
}

#pragma endregion

//* AI
#pragma region AI

#define AI_MAX_WORKING_QUEUE_SIZE 1

AI_Move current_bestMove = AI_Move(INT_MIN, 0, 0);

void updateAI()
{
    if (lastAIFrame == frameCount || frameCount % AI_FRAMES_PER_INPUT != 0)
        return;

    lastAIFrame = frameCount;

    softDrop = false;

#ifdef _AI_TRAINING_MODE
    while (currentTetromino.currentRotationState < current_bestMove.rotationState)
    {
        if (!currentTetromino.tryRotate(1))
        {
            if (currentTetromino.canMove(down))
                currentTetromino.move(down);
            else
                break;
        }
    }

    while (current_bestMove.leftMostCell < currentTetromino.getLeftMost().x && currentTetromino.canMove(left))
        currentTetromino.move(left);

    while (current_bestMove.leftMostCell > currentTetromino.getLeftMost().x && currentTetromino.canMove(right))
        currentTetromino.move(right);

    hardDrop();
    finishInput(true);
#else
    bool successfulMovement = false;
    int leftMostCell = currentTetromino.getLeftMost().x;

    if (currentTetromino.currentRotationState < current_bestMove.rotationState)
        successfulMovement = rotateInput(1);
    else if (current_bestMove.leftMostCell < leftMostCell)
        successfulMovement = moveInput(-1);
    else if (current_bestMove.leftMostCell > leftMostCell)
        successfulMovement = moveInput(1);
    else
        softDrop = true;

    finishInput(successfulMovement);
#endif
}

void resetAndRotateTestBlock(Tetromino &testTetromino, int rotation)
{
    testTetromino.clearPixels();
    testTetromino.resetPosition();
    testTetromino.jumpToSpawnPosition();

    while (testTetromino.currentRotationState != rotation)
    {
        if (!testTetromino.tryRotate(1))
        {
            if (testTetromino.canMove(down))
                testTetromino.move(down);
            else
                break;
        }
    }
}

AI_Move updateBestMove(Tetromino &testTetromino, int holdQueueIndex)
{
    //Tetromino testTetromino = currentTetromino;
    testTetromino.placePixels();

    int maxRotations = ai_maxRotationsForShapeType[testTetromino.shape.shapeType];

    AI_Move bestMove = AI_Move(INT_MIN, 0, 0);

    for (int rotation = 0; rotation < maxRotations; rotation++)
    {
        resetAndRotateTestBlock(testTetromino, rotation);

        //*Move all the way to the left
        int x = 0;
        for (; testTetromino.canMove(point(x - 1, 0)); x--)
        {
        }

        //*Go through each possible position within this rotation (only from the top downward to avoid too many options)
        for (; x < DISPLAY_BOARD_WIDTH; x++)
        {
            //*Try to move to the x point if we can, if not we are done with this shape in this rotation
            if (testTetromino.canMove(point(x, 0)))
                testTetromino.move(point(x, 0));
            else
                break;

            while (testTetromino.canMove(down))
                testTetromino.move(down);

            float moveScore = 0;
            // if (holdQueueIndex == AI_MAX_WORKING_QUEUE_SIZE - 1)
            // {
            moveScore = calculateHeuristicScore(aggregateHeight(), countLineClears(), countHoles(), calculateBumpiness());
            // }
            // else
            // {
            //     int nextQueue = holdQueueIndex + 1;
            //     Tetromino next = allTetrominoTypes[tetrominoQueue[(queueIndex + nextQueue) % (TETROMINO_COUNT * 2)]];
            //     moveScore = updateBestMove(next, nextQueue).moveScore;
            // }

#if defined _DEBUG
            FastLED.show();
            delay(10);
#endif

            if (moveScore > bestMove.moveScore)
            {
                bestMove.moveScore = moveScore;
                bestMove.rotationState = testTetromino.currentRotationState;
                bestMove.leftMostCell = testTetromino.getLeftMost().x;
#if defined _DEBUG
                delay(500);
#endif
            }

            //*Reset and rotate for next movement
            testTetromino.clearPixels();
            testTetromino.resetPosition();
            testTetromino.jumpToSpawnPosition();

            resetAndRotateTestBlock(testTetromino, rotation);
        }
    }

    testTetromino.clearPixels();
    return bestMove;
}

void onAISpawn()
{
    lastAIFrame = frameCount;
    currentTetromino.clearPixels();

    Tetromino testTetromino = currentTetromino;
    current_bestMove = updateBestMove(testTetromino, -1);

#if defined _DEBUG && !defined _AI_TRAINING_MODE
    Serial.print("SCORE: ");
    Serial.println(current_bestMove.moveScore);

    Serial.print("ROTATION: ");
    Serial.println(current_bestMove.rotationState);

    Serial.print("LEFT ALIGNED POSITION");
    Serial.println(current_bestMove.leftMostCell);
    Serial.println("-----------------");
#endif

    currentTetromino.placePixels();
}

float calculateHeuristicScore(float aggregateHeight, float completeLines, float holes, float bumpiness)
{
    return weight_aggregateHeight * aggregateHeight + weight_completeLines * completeLines + weight_holes * holes + weight_bumpiness * bumpiness;
}

int columnHeight(int column)
{
    //*Work from the top to the bottom, return the first occupied block as our height
    for (int y = DISPLAY_BOARD_HEIGHT - 1; y >= 0; y--)
    {
        if (isOccupied(point(column, y)))
            return y + 1;
    }

    return 0;
}

//* This heuristic tells us how “high” a grid is.
//* To compute the aggregate height, we take the sum of the height of each column (the distance from the highest tile in each column to the bottom of the grid).
int aggregateHeight()
{
    int height = 0;
    for (int x = 0; x < DISPLAY_BOARD_WIDTH; x++)
    {
        height += columnHeight(x);
    }
    return height;
}

//*The bumpiness of a grid tells us the variation of its column heights. It is computed by summing up the absolute differences between all two adjacent columns.
int calculateBumpiness()
{
    int total = 0;
    for (int x = 0; x < DISPLAY_BOARD_WIDTH - 1; x++)
    {
        total += abs(columnHeight(x) - columnHeight(x + 1));
    }
    return total;
}

//* A hole is defined as an empty space such that there is at least one tile in the same column above it.
int countHoles()
{
    int holes = 0;

    for (int x = 0; x < DISPLAY_BOARD_WIDTH; x++)
    {
        bool blocked = false;
        //*Work from the top to the bottom, once we find the first occupied block, count any empties under it
        for (int y = DISPLAY_BOARD_HEIGHT - 1; y >= 0; y--)
        {
            if (isOccupied(point(x, y)))
                blocked = true;
            else if (blocked)
                holes++;
        }
    }

    return holes;
}

bool isCompleteLine(int row)
{
    for (int x = 0; x < DISPLAY_BOARD_WIDTH; x++)
    {
        if (!isOccupied(point(x, row)))
        {
            return false;
        }
    }

    return true;
}

//* Simply the the number of complete lines in a grid.
int countLineClears()
{
    int clearedRowCount = 0;
    for (int y = 0; y < DISPLAY_BOARD_HEIGHT; y++)
    {
        if (isCompleteLine(y))
            clearedRowCount++;
    }

    return clearedRowCount;
}

void debugAIHeuristics()
{
    for (int x = 0; x < DISPLAY_BOARD_WIDTH; x++)
    {
        Serial.print(columnHeight(x));
        Serial.print(" | ");
    }
    Serial.print('\n');

    Serial.print("Aggregate Height: ");
    Serial.println(aggregateHeight());

    Serial.print("Bumpiness: ");
    Serial.println(calculateBumpiness());

    Serial.print("Holes: ");
    Serial.println(countHoles());
    Serial.println("-------------------");
}

#pragma endregion
