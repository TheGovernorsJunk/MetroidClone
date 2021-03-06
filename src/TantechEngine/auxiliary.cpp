#include "auxiliary.h"
#include <SDL_rect.h>
#include "types.h"

namespace te
{

    Vector2i getCenter(const SDL_Rect& rect)
    {
        Vector2i center = {
            rect.x + (rect.w / 2),
            rect.y + (rect.h / 2)
        };
        return center;
    }

    void adjustViewport(int screenWidth, int screenHeight, float targetAspectRatio, void(__stdcall* viewportFn)(GLint, GLint, GLsizei, GLsizei))
    {
        int width = screenWidth;
        int height = (int)(width / targetAspectRatio + 0.5f);

        if (height > screenHeight)
        {
            height = screenHeight;
            width = (int)(height * targetAspectRatio + 0.5);
        }

        int vp_x = (screenWidth / 2) - (width / 2);
        int vp_y = (screenHeight / 2) - (height / 2);

        viewportFn(vp_x, vp_y, width, height);
    }

    //void handlePaddleCollision(Rectangle& ball, const Rectangle& paddle, float dt, float velocityScalar)
    //{
    //    if (checkCollision(ball, paddle))
    //    {
    //        Vector2i paddleCenter = getCenter(paddle);
    //        Vector2i ballCenter = getCenter(ball);
    //        Vector2f paddleToBall = {
    //            (float)ballCenter.x - paddleCenter.x,
    //            (float)ballCenter.y - paddleCenter.y
    //        };
    //        float length = std::sqrt((paddleToBall.x * paddleToBall.x) + (paddleToBall.y * paddleToBall.y));
    //        Vector2f velocity = {
    //            velocityScalar * paddleToBall.x / length,
    //            velocityScalar * paddleToBall.y / length
    //        };
    //        ball.setVelocity(velocity);
    //        ball.update(dt);
    //    }
    //}

    //void handleWallCollision(Rectangle& ball, const Rectangle& wall, float dt)
    //{
    //    if (checkCollision(ball, wall))
    //    {
    //        Vector2f vel = ball.getVelocity();
    //        SDL_Rect intersection = getIntersection(ball, wall);
    //        if (intersection.w > intersection.h)
    //        {
    //            vel.y = -vel.y;
    //            ball.setVelocity(vel);
    //        }
    //        else
    //        {
    //            vel.x = -vel.x;
    //            ball.setVelocity(vel);
    //        }
    //    }
    //}
}
