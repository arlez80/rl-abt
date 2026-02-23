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

#include <raylib.h>

#ifndef YK_ANIMATION_H
#define YK_ANIMATION_H

// アニメーションブレンドノード設定（前方宣言）
typedef struct AnimationBlendNode AnimationBlendNode;

// ボーンのレスト情報
typedef struct BoneRestInfo {
    Vector3 location;               // 親からの距離

    Quaternion invRotation;         // 逆回転
    Quaternion poseToParent;        // ポーズ空間から親空間への変換
    Quaternion parentToPose;        // 親空間からポーズ空間への変換
} BoneRestInfo;

// アニメーションツリー用アニメーションデータ
typedef struct ConvertedAnimationData {
    char name[32];                  // アニメーション名
    Transform** framePoses;         // ボーン単位の回転を記録しているリスト
    int frameCount;                 // フレーム数
} ConvertedAnimation;

// アニメーションブレンドノード用のループモード
typedef enum {
    ALM_ONE_SHOT,                   // 1回のみ
    ALM_LOOP,                       // ループ
} AnimationLoopMode;

// アニメーション合成ノード種類
typedef enum {
    ABNT_SOURCE,                            // アニメーション供給元
    ABNT_LEAP,                              // 線形補間
    ABNT_ADD,                               // 加算
    ABNT_TRASITION,                         // 遷移
} AnimationBlendNodeType;

// アニメーション合成結果
typedef struct AnimationBlendResult {
    Transform* pose;                        // 合成結果
} AnimationBlendResult;

typedef struct AnimationBlendNodeSource {
    const char* name;                           // アニメーション名
    AnimationLoopMode loopMode;                 // アニメーションループモード

    ConvertedAnimation* convertedAnimation;     // アニメーションソース
    float position;                             // 再生位置
    float playSpeed;                            // 再生速度
    float length;                               // 総再生時間
} AnimationBlendNodeSource;

// 2つの入力があるアニメーションノード
typedef struct AnimationBlendNodeInput2 {
    AnimationBlendNode* input;              // 入力
    AnimationBlendNode* mixInput;           // 合成
    float weight;                           // ウェイト
    const char** filteredBones;             // ボーンフィルタリスト
    int filteredBoneCount;                  // ボーンフィルタ数
} AnimationBlendNodeInput2;

// 遷移アニメーションノード
typedef struct AnimationBlendNodeTransition {
    AnimationBlendNode* inputs;             // 入力
    int inputCount;                         // 入力数

    int index;                              // 現在の入力元
    int nextIndex;                          // 遷移先
    float transitionSeconds;                // 総遷移時間
    float position;                         // 遷移中時間
} AnimationBlendNodeTransition;

// アニメーション合成ノード
typedef struct AnimationBlendNode {
    int uniqueId;                           // 検索用ユニークID
    AnimationBlendNodeType type;            // ノード種類

    bool playedPreviousEvaluate;            // 前のEvaluateで演算したか？

    union {
        AnimationBlendNodeSource source;            // アニメーション元用
        AnimationBlendNodeInput2 lerp;              // 線形補間用
        AnimationBlendNodeInput2 add;               // 加算用
        AnimationBlendNodeTransition transition;    // トラジション用
    };
} AnimationBlendNode;

// アニメーション合成木
typedef struct AnimationBlendTree {
    AnimationBlendNode* root;					// ルートノード

    bool validated;								// 有効確認済み？

    int animationCount;							// アニメーション個数
    ModelAnimation* animations;					// 元アニメーションデータ
    ConvertedAnimation* convertedAnimations;	// 変換済みアニメーションデータ
    int boneCount;								// ボーン個数
    BoneInfo* bones;							// ボーン情報
    BoneRestInfo* boneRests;					// ボーンのレスト情報

    // 合成結果返却用
    Transform* convertedPose;					// Model用の変換済みポーズ
    Transform** transferPointer;				// 転送用ポインタ

    // 合成用バッファ
    AnimationBlendResult* stack;				// 合成結果スタック
    int stackPointer;							// スタックポインタ
} AnimationBlendTree;

RLAPI AnimationBlendTree LoadAnimationBlendTree(Model model, ModelAnimation* animations, int animationCount, AnimationBlendNode* root);
RLAPI void UnloadAnimationBlendTree(AnimationBlendTree tree);
RLAPI AnimationBlendNode* FindAnimationBlendNodeByUniqueID(AnimationBlendTree tree, int uniqueId);
RLAPI ModelAnimation EvalAnimationBlendTree(AnimationBlendTree tree, float delta);

#endif // YK_ANIMATION_H
