# Urho3D SpriteBatch

In my tests Urho3D DX9 version has identical perfomance with XNA (DirectX 9) and Urho3D DX11 with MonoGame (DirectX 11).<br>

License: Public Domain

See also: https://github.com/1vanK/Urho3DBitmapFontGenerator

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
    spriteBatch_->Draw(texture, Vector2(100, 100));
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
    spriteBatch_->Draw(texture, Vector2(0, -10));
    spriteBatch_->End();
}
```
![Screenshot2](https://github.com/1vanK/Urho3DSpriteBatch/raw/master/Screen02.png)
```
void Start()
{
    ...
    spriteBatch_ = new SpriteBatch(context_);
    spriteBatch_->virtualScreenSize_ = IntVector2(800, 600);
}
```
![Screenshot3](https://github.com/1vanK/Urho3DSpriteBatch/raw/master/Screen03.png)

You can change portition size (max count of sprites per Draw Call):
```
spriteBatch_ = new SpriteBatch(context_, 600);
```
The optimal value depends very much on used API and specific PC.
