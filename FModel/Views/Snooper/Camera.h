#pragma once
// Ported from FModel/Views/Snooper/Camera.cs — the WorldMode enum only.
//
// Snooper is FModel's OpenGL 3D viewer; none of it is ported yet. WorldMode is here on its own because
// UserSettings persists CameraMode, and the settings layer must not wait on the renderer.

namespace FModel::Views::Snooper
{
    class Camera
    {
    public:
        enum class WorldMode
        {
            FlyCam,
            Arcball
        };
    };
}
