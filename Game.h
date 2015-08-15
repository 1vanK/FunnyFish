#pragma once


class Game : public Application
{
    OBJECT(Game);

public:
    Game(Context* context);

    virtual void Setup();
    virtual void Start();

protected:
    SharedPtr<Scene> scene_;
    float yaw_;
    float pitch_;

private:
	void CreateScene();
	void SetupViewport();
	void MoveCamera(float timeStep);
	void SubscribeToEvents();
	void HandleUpdate(StringHash eventType, VariantMap& eventData);

	void HandleMouseButtonDown(StringHash eventType, VariantMap& eventData);
	void HandleMouseButtonUp(StringHash eventType, VariantMap& eventData);

	bool Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable, bool toInvisPlane);
    Ray GetCameraRay();

};
