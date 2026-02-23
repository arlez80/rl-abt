/*
    アニメーション合成ツリー
        Programed by きのもと 結衣 @arlez80

    MIT License

    Copyright (c) 2026 きのもと 結衣

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <string.h>
#include <memory.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>

#include "animation.h"

#define MAX_BLEND_RESULT_STACK 32

static ModelAnimation* FindModelAnimation(AnimationBlendTree* abt, const char* name)
{
    for (int i = 0; i < abt->animationCount; i++) {
        if (strncmp(&abt->animations[i].name[0], name, sizeof(abt->animations[i].name)) == 0) {
            return &abt->animations[i];
        }
    }

    return NULL;
}

static ConvertedAnimation* FindConvertedAnimationData(AnimationBlendTree* abt, const char* name)
{
    for (int i = 0; i < abt->animationCount; i++) {
        if (strncmp(&abt->convertedAnimations[i].name[0], name, sizeof(abt->convertedAnimations[i].name)) == 0) {
            return &abt->convertedAnimations[i];
        }
    }

    return NULL;
}

static AnimationBlendNode* FindNodeByUniqueID(AnimationBlendNode* node, int uniqueId)
{
    if (node->uniqueId == uniqueId) return node;

    switch (node->type) {
    case ABNT_ADD: {
        AnimationBlendNode* inputResult = FindNodeByUniqueID(node->add.input, uniqueId);
        if (inputResult) return inputResult;
        AnimationBlendNode* mixInputResult = FindNodeByUniqueID(node->add.mixInput, uniqueId);
        if (mixInputResult) return mixInputResult;
    } break;

    case ABNT_LEAP: {
        AnimationBlendNode* inputResult = FindNodeByUniqueID(node->lerp.input, uniqueId);
        if (inputResult) return inputResult;
        AnimationBlendNode* mixInputResult = FindNodeByUniqueID(node->lerp.mixInput, uniqueId);
        if (mixInputResult) return mixInputResult;
    } break;

    case ABNT_SOURCE: {
        // なし
    } break;

    case ABNT_TRASITION: {
        for (int i = 0; i < node->transition.inputCount; i++) {
            AnimationBlendNode* result = FindNodeByUniqueID(&node->transition.inputs[i], uniqueId);
            if (result) return result;
        }
    } break;
    }

    return NULL;
}

static int FindBoneByName(AnimationBlendTree* abt, const char* name)
{
    for (int i = 0; i < abt->boneCount; i++) {
        if (strncmp(abt->bones[i].name, name, sizeof(abt->bones[i].name)) == 0) {
            return i;
        }
    }

    return -1;
}

static ConvertedAnimation LoadConvertedAnimationData(AnimationBlendTree* abt, ModelAnimation animation)
{
    ConvertedAnimation cad = { 0 };

    strncpy_s(&cad.name[0], sizeof(cad.name), &animation.name[0], sizeof(animation.name));
    cad.frameCount = animation.frameCount;
    cad.framePoses = MemAlloc(sizeof(Transform*) * animation.frameCount);
    for (int frame = 0; frame < animation.frameCount; frame++) {
        Transform* poses = MemAlloc(sizeof(Transform) * animation.boneCount);

        for (int i = 0; i < animation.boneCount; i++) {
            Transform result = { 0 };

            Transform pose = animation.framePoses[frame][i];
            BoneInfo bone = abt->bones[i];
            BoneRestInfo boneRest = abt->boneRests[i];
            const int parent = bone.parent;

            if (parent != -1) {
                Transform parentPose = animation.framePoses[frame][parent];
                result.translation = (
                    Vector3Subtract(
                        Vector3RotateByQuaternion(
                            Vector3Subtract(pose.translation, parentPose.translation)
                            , QuaternionInvert(parentPose.rotation)
                        )
                        , boneRest.location
                    )
                    );
                result.rotation = QuaternionMultiply(
                    abt->boneRests[i].parentToPose
                    , QuaternionMultiply(
                        QuaternionInvert(parentPose.rotation)
                        , pose.rotation
                    )
                );
                result.scale = Vector3Divide(pose.scale, parentPose.scale);
            }
            else {
                result = pose;
            }

            poses[i] = result;
        }

        cad.framePoses[frame] = poses;
    }

    return cad;
}

static void UnloadConvertedAnimationData(ConvertedAnimation cad)
{
    for (int i = 0; i < cad.frameCount; i++) {
        MemFree(cad.framePoses[i]);
    }
    MemFree(cad.framePoses);
}

static bool CheckValidate(AnimationBlendTree* abt, AnimationBlendNode* node)
{
    switch (node->type) {
    case ABNT_ADD: {
        if (!node->add.input && !node->add.mixInput) return false;
        if (!CheckValidate(abt, node->add.input)) return false;
        if (!CheckValidate(abt, node->add.mixInput)) return false;
    } break;

    case ABNT_LEAP: {
        if (!node->lerp.input && !node->lerp.mixInput) return false;
        if (!CheckValidate(abt, node->lerp.input)) return false;
        if (!CheckValidate(abt, node->lerp.mixInput)) return false;
    } break;

    case ABNT_SOURCE: {
        if (!node->source.name) return false;
        ModelAnimation* ma = FindModelAnimation(abt, node->source.name);
        if (!ma) return false;
    } break;

    case ABNT_TRASITION: {
        if (node->transition.inputCount <= 0) return false;
        for (int i = 0; i < node->transition.inputCount; i++) {
            if (!CheckValidate(abt, &node->transition.inputs[i])) return false;
        }
    } break;
    }

    return true;
}

static void PreprocessTree(AnimationBlendTree* abt, AnimationBlendNode* node)
{
    switch (node->type) {
    case ABNT_ADD: {
        PreprocessTree(abt, node->add.input);
        PreprocessTree(abt, node->add.mixInput);
        node->add.weight = 0.0f;
    } break;

    case ABNT_LEAP: {
        PreprocessTree(abt, node->lerp.input);
        PreprocessTree(abt, node->lerp.mixInput);
        node->lerp.weight = 0.0f;
    } break;

    case ABNT_SOURCE: {
        node->source.convertedAnimation = FindConvertedAnimationData(abt, node->source.name);
        node->source.position = 0.0f;
        node->source.playSpeed = 1.0f;
        node->source.length = node->source.convertedAnimation->frameCount / 60.0f;
    } break;

    case ABNT_TRASITION: {
        node->transition.index = 0;
        node->transition.nextIndex = -1;
        node->transition.position = 0.0f;
        node->transition.transitionSeconds = 0.05f;
        for (int i = 0; i < node->transition.inputCount; i++) {
            PreprocessTree(abt, &node->transition.inputs[i]);
        }
    } break;
    }
}

static void PreEvaluate(AnimationBlendNode* node)
{
    switch (node->type) {
    case ABNT_ADD: {
        PreEvaluate(node->lerp.input);
        PreEvaluate(node->lerp.mixInput);
    } break;

    case ABNT_LEAP: {
        PreEvaluate(node->lerp.input);
        PreEvaluate(node->lerp.mixInput);
    } break;

    case ABNT_SOURCE: {
        if (!node->playedPreviousEvaluate) {
            node->source.position = 0.0;
        }
    } break;

    case ABNT_TRASITION: {
        for (int i = 0; i < node->transition.inputCount; i++) {
            PreEvaluate(&node->transition.inputs[i]);
        }
    } break;
    }

    node->playedPreviousEvaluate = false;
}

static void Evaluate(AnimationBlendTree* abt, AnimationBlendNode* node, float delta)
{
    switch (node->type) {
    case ABNT_ADD: {
        Evaluate(abt, node->add.input, delta);
        AnimationBlendResult* result = &abt->stack[abt->stackPointer++];
        Evaluate(abt, node->add.mixInput, delta);
        AnimationBlendResult* mixResult = &abt->stack[abt->stackPointer--];

        const float t = node->add.weight;

        for (int i = 0; i < abt->boneCount; i++) {
            // TODO: あらかじめ計算済みを用意するか？
            if (node->add.filteredBones) {
                bool found = false;
                for (int k = 0; k < node->add.filteredBoneCount; k++) {
                    if (0 <= FindBoneByName(abt, node->add.filteredBones[k])) {
                        found = true;
                        break;
                    }
                }
                if (found) continue;
            }

            result->pose[i].translation = Vector3Lerp(
                result->pose[i].translation,
                mixResult->pose[i].translation,
                t
            );
            result->pose[i].rotation = QuaternionMultiply(
                QuaternionSlerp(
                    QuaternionIdentity(),
                    mixResult->pose[i].rotation,
                    t
                ),
                result->pose[i].rotation
            );
            // TODO: 後で対応する
            /*result->pose[i].scale = Vector3Lerp(
                result->pose[i].scale,
                mixResult->pose[i].scale,
                t
            );*/
        }
    } break;

    case ABNT_LEAP: {
        Evaluate(abt, node->lerp.input, delta);
        AnimationBlendResult* result = &abt->stack[abt->stackPointer++];
        Evaluate(abt, node->lerp.mixInput, delta);
        AnimationBlendResult* mixResult = &abt->stack[abt->stackPointer--];

        const float t = node->lerp.weight;

        for (int i = 0; i < abt->boneCount; i++) {
            // TODO: あらかじめ計算済みを用意するか？
            if (node->lerp.filteredBones) {
                bool found = false;
                for (int k = 0; k < node->lerp.filteredBoneCount; k++) {
                    if (0 <= FindBoneByName(abt, node->lerp.filteredBones[k])) {
                        found = true;
                        break;
                    }
                }
                if (found) continue;
            }

            result->pose[i].translation = Vector3Lerp(
                result->pose[i].translation,
                mixResult->pose[i].translation,
                t
            );
            result->pose[i].rotation = QuaternionSlerp(result->pose[i].rotation, mixResult->pose[i].rotation, t);
            result->pose[i].scale = Vector3Lerp(
                result->pose[i].scale,
                mixResult->pose[i].scale,
                t
            );
        }
    } break;

    case ABNT_SOURCE: {
        AnimationBlendResult* result = &abt->stack[abt->stackPointer];
        const int frame = (int)Clamp(
            node->source.position * 60.0f,
            0,
            node->source.convertedAnimation->frameCount - 1
        );

        for (int i = 0; i < abt->boneCount; i++) {
            result->pose[i] = node->source.convertedAnimation->framePoses[frame][i];
        }

        switch (node->source.loopMode) {
        case ALM_ONE_SHOT:
            node->source.position = fminf(node->source.position + delta * node->source.playSpeed, 1.0);
            break;
        case ALM_LOOP:
            node->source.position = fmodf(node->source.position + delta * node->source.playSpeed, node->source.length);
            break;
        }

    } break;

    case ABNT_TRASITION: {
        Evaluate(abt, &node->transition.inputs[node->transition.index], delta);
        AnimationBlendResult* result = &abt->stack[abt->stackPointer++];

        if (0 <= node->transition.nextIndex) {
            Evaluate(abt, &node->transition.inputs[node->transition.nextIndex], delta);
            AnimationBlendResult* mixResult = &abt->stack[abt->stackPointer--];

            const float t = node->transition.position / fmaxf(node->transition.transitionSeconds, EPSILON);

            for (int i = 0; i < abt->boneCount; i++) {
                result->pose[i].translation = Vector3Lerp(
                    result->pose[i].translation
                    , mixResult->pose[i].translation
                    , t
                );
                result->pose[i].rotation = QuaternionSlerp(result->pose[i].rotation, mixResult->pose[i].rotation, t);
                result->pose[i].scale = Vector3Lerp(
                    result->pose[i].scale
                    , mixResult->pose[i].scale
                    , t
                );
            }

            node->transition.position += delta;
            if (node->transition.transitionSeconds <= node->transition.position) {
                node->transition.position = 0.0;
                node->transition.index = node->transition.nextIndex;
                node->transition.nextIndex = -1;
            }
        }
    } break;
    }

    node->playedPreviousEvaluate = true;
}

RLAPI AnimationBlendTree LoadAnimationBlendTree(Model model, ModelAnimation* animations, int animationCount, AnimationBlendNode* root)
{
    AnimationBlendTree abt = { 0 };
    if (model.boneCount == 0) {
        return abt;
    }

    //
    abt.boneCount = model.boneCount;
    abt.bones = model.bones;
    abt.boneRests = MemAlloc(sizeof(BoneRestInfo) * model.boneCount);
    for (int i = 0; i < model.boneCount; i++) {
        const int parent = model.bones[i].parent;
        abt.boneRests[i].invRotation = QuaternionInvert(model.bindPose[i].rotation);

        if (parent != -1) {
            abt.boneRests[i].location = Vector3RotateByQuaternion(
                Vector3Subtract(model.bindPose[i].translation, model.bindPose[parent].translation)
                , abt.boneRests[parent].invRotation
            );
            abt.boneRests[i].parentToPose = QuaternionMultiply(
                abt.boneRests[i].invRotation
                , model.bindPose[parent].rotation
            );
            abt.boneRests[i].poseToParent = QuaternionMultiply(
                abt.boneRests[parent].invRotation
                , model.bindPose[i].rotation
            );
        }
        else {
            abt.boneRests[i].location = (Vector3){ 0.0f, 0.0f, 0.0f };
        }
    }

    // アニメーション事前処理
    abt.animationCount = animationCount;
    abt.animations = animations;
    abt.convertedAnimations = MemAlloc(sizeof(ConvertedAnimation) * animationCount);
    for (int i = 0; i < animationCount; i++) {
        abt.convertedAnimations[i] = LoadConvertedAnimationData(&abt, animations[i]);
    }

    // スタック
    abt.stack = MemAlloc(sizeof(AnimationBlendResult) * MAX_BLEND_RESULT_STACK);
    for (int i = 0; i < MAX_BLEND_RESULT_STACK; i++) {
        abt.stack[i].pose = MemAlloc(sizeof(Transform) * model.boneCount);
    }

    // 返却用ポーズバッファ
    abt.convertedPose = MemAlloc(sizeof(Transform) * model.boneCount);
    abt.transferPointer = MemAlloc(sizeof(Transform*) * 1);
    abt.transferPointer[0] = abt.convertedPose;

    // ツリー確認
    abt.root = root;
    abt.validated = CheckValidate(&abt, root);
    if (abt.validated) {
        PreprocessTree(&abt, root);
    }

    return abt;
}

RLAPI void UnloadAnimationBlendTree(AnimationBlendTree abt)
{
    if (abt.boneRests == NULL) {
        return;
    }

    MemFree(abt.boneRests);

    for (int i = 0; i < abt.animationCount; i++) {
        UnloadConvertedAnimationData(abt.convertedAnimations[i]);
    }
    MemFree(abt.convertedAnimations);

    for (int i = 0; i < MAX_BLEND_RESULT_STACK; i++) {
        MemFree(abt.stack[i].pose);
    }
    MemFree(abt.stack);

    MemFree(abt.convertedPose);
    MemFree(abt.transferPointer);
}

RLAPI AnimationBlendNode* FindAnimationBlendNodeByUniqueID(AnimationBlendTree abt, int uniqueId)
{
    return FindNodeByUniqueID(abt.root, uniqueId);
}

RLAPI ModelAnimation EvalAnimationBlendTree(AnimationBlendTree abt, float delta)
{
    ModelAnimation ma = { 0 };

    if (!abt.validated) {
        return ma;
    }

    PreEvaluate(abt.root);
    abt.stackPointer = 0;
    Evaluate(&abt, abt.root, delta);

    AnimationBlendResult result = abt.stack[0];
    for (int i = 0; i < abt.boneCount; i++) {
        const BoneRestInfo boneRest = abt.boneRests[i];
        const int parent = abt.bones[i].parent;

        if (parent == -1) {
            // 根っこ
            abt.convertedPose[i] = result.pose[i];
        }
        else {
            // 子供
            abt.convertedPose[i].translation = Vector3Add(
                Vector3Add(
                    abt.convertedPose[parent].translation
                    , result.pose[i].translation
                )
                , Vector3RotateByQuaternion(
                    Vector3Multiply(boneRest.location, abt.convertedPose[parent].scale)
                    , abt.convertedPose[parent].rotation
                )
            );
            abt.convertedPose[i].scale = Vector3Multiply(result.pose[i].scale, abt.convertedPose[parent].scale);
            abt.convertedPose[i].rotation = QuaternionMultiply(
                abt.convertedPose[parent].rotation
                , QuaternionMultiply(
                    boneRest.poseToParent
                    , result.pose[i].rotation
                )
            );
        }
    }

    ma.frameCount = 1;
    ma.boneCount = abt.boneCount;
    ma.bones = abt.bones;
    ma.framePoses = abt.transferPointer;

    return ma;
}


