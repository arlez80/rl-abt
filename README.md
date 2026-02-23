# Animation Blend Tree for Raylib

raylib用のアニメーション合成ツリー。

# API

```
RLAPI AnimationBlendTree LoadAnimationBlendTree(Model model, ModelAnimation* animations, int animationCount, AnimationBlendNode* root);
RLAPI void UnloadAnimationBlendTree(AnimationBlendTree tree);
RLAPI AnimationBlendNode* FindAnimationBlendNodeByUniqueID(AnimationBlendTree tree, int uniqueId);
RLAPI ModelAnimation EvalAnimationBlendTree(AnimationBlendTree tree, float delta);
```

# Node Types

- Add
- Lerp
- Transition
- Source

# License

MIT License

# Author

あるる（きのもと 結衣） @arlez80
