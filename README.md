# Urho3D SpriteBatch

(I do not know why, but OpenGL version works faster than DirectX 9 on my PC).

License: Public Domain

```
void Start()
{
    ...
    spriteBatch_ = new SpriteBatch(context_);
}

void HandleEndViewRender(StringHash eventType, VariantMap& eventData)
{
    Texture2D* texture = GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Urho2D/Ball.png");
    spriteBatch_->Begin();
    spriteBatch_->Draw(texture, Vector2(100, 100), nullptr, Color::WHITE, Vector2(20, 20), 180.0f);
    spriteBatch_->End();
}
```
![Screenshot1](https://github.com/1vanK/Urho3DSpriteBatch/raw/master/Screen01.png)
```
void HandleEndViewRender(StringHash eventType, VariantMap& eventData)
{
    Camera* camera = scene_->GetChild("Camera")->GetComponent<Camera>();
    Texture2D* texture = GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Urho2D/Ball.png");
    spriteBatch_->Begin(BLEND_ALPHA, CMP_LESSEQUAL, 0.0f, camera);
    spriteBatch_->Draw(texture, Vector2(1, 1), nullptr, Color::WHITE, Vector2(20, 20), 180.0f);
    spriteBatch_->End();
}
```
![Screenshot2](https://github.com/1vanK/Urho3DSpriteBatch/raw/master/Screen02.png)
