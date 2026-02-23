/*
    テストプログラム
*/

#include <stdio.h>
#include <raylib.h>
#include "animation.h"

enum {
    NODE_ID_TRANSITION = 1,
    NODE_ID_ADD,
    NODE_ID_LERP,
};

int main(int argc, char* argv[])
{
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "test");

    SetTargetFPS(60);

    Camera camera = { 0 };
    camera.position = (Vector3){ 0.0f, 1.5f, 7.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model model = LoadModel("./resources/test.glb");
    int animationCount;
    ModelAnimation* animation = LoadModelAnimations("./resources/test.glb", &animationCount);

    AnimationBlendNode root = {
        .uniqueId = NODE_ID_TRANSITION,
        .type = ABNT_TRASITION,
        .transition = {
            .inputs = (AnimationBlendNode[]) {
                {
                    .uniqueId = -1,
                    .type = ABNT_SOURCE,
                    .source = {
                        .name = "init",
                        .loopMode = ALM_LOOP,
                    }
                },
                {
                    .uniqueId = NODE_ID_ADD,
                    .type = ABNT_ADD,
                    .add = {
                        .input = (AnimationBlendNode[]) {{
                            .uniqueId = -1,
                            .type = ABNT_SOURCE,
                            .source = {
                                .name = "mix_a",
                                .loopMode = ALM_ONE_SHOT,
                            }
                        }},
                        .mixInput = (AnimationBlendNode[]) {{
                            .uniqueId = -1,
                            .type = ABNT_SOURCE,
                            .source = {
                                .name = "mix_b",
                                .loopMode = ALM_ONE_SHOT,
                            }
                        }}
                    }
                },
                {
                    .uniqueId = NODE_ID_LERP,
                    .type = ABNT_LEAP,
                    .add = {
                        .input = (AnimationBlendNode[]) {{
                            .uniqueId = -1,
                            .type = ABNT_SOURCE,
                            .source = {
                                .name = "mix_a",
                                .loopMode = ALM_ONE_SHOT,
                            }
                        }},
                        .mixInput = (AnimationBlendNode[]) {{
                            .uniqueId = -1,
                            .type = ABNT_SOURCE,
                            .source = {
                                .name = "mix_b",
                                .loopMode = ALM_ONE_SHOT,
                            }
                        }}
                    }
                }
            },
            .inputCount = 3,
        }
    };

    AnimationBlendTree abt = LoadAnimationBlendTree(model, animation, animationCount, &root);
    AnimationBlendNode* nodeTransition = FindAnimationBlendNodeByUniqueID(abt, NODE_ID_TRANSITION);
    AnimationBlendNode* nodeAdd = FindAnimationBlendNodeByUniqueID(abt, NODE_ID_ADD);
    AnimationBlendNode* nodeLerp = FindAnimationBlendNodeByUniqueID(abt, NODE_ID_LERP);

    const char* transition_name[] = {
        "init",
        "add",
        "lerp",
    };

    while (!WindowShouldClose()) {
        //UpdateCamera(&camera, CAMERA_ORBITAL);

        if (IsKeyPressed('Z')) {
            nodeTransition->transition.nextIndex = 0;
        }
        if (IsKeyPressed('X')) {
            nodeTransition->transition.nextIndex = 1;
        }
        if (IsKeyPressed('C')) {
            nodeTransition->transition.nextIndex = 2;
        }
        if (IsKeyPressed('A')) {
            nodeAdd->add.weight = 0.0f;
            nodeLerp->lerp.weight = 0.0f;
        }
        if (IsKeyPressed('S')) {
            nodeAdd->add.weight = 1.0f;
            nodeLerp->lerp.weight = 1.0f;
        }

        ModelAnimation ma = EvalAnimationBlendTree(abt, 1.0f / 60.0f);
        UpdateModelAnimation(model, ma, 0);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        //DrawModel(model, (Vector3) { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
        DrawModelWires(model, (Vector3) { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
        DrawGrid(10, 1.0f);
        EndMode3D();

        char buffer[256];
        sprintf(&buffer[0], "[Z][X][C] Transition: %s", transition_name[nodeTransition->transition.index]);
        DrawText(&buffer[0], 8, 8, 20, BLACK);
        sprintf(&buffer[0], "[A][S] Add/Lerp Weight: %8.3f", nodeAdd->add.weight);
        DrawText(&buffer[0], 8, 32, 20, BLACK);

        EndDrawing();
    }

    UnloadAnimationBlendTree(abt);
    UnloadModelAnimations(animation, animationCount);
    UnloadModel(model);
    CloseWindow();

    return 0;
}
