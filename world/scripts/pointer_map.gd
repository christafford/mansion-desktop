## The displayed texture is already upright; normalize its drawn rect directly
## into committed logical surface coordinates (not raw buffer coordinates).
extends RefCounted

static func drawn_rect(view_rect: Rect2, texture_size: Vector2) -> Rect2:
	if texture_size.x <= 0 or texture_size.y <= 0 or view_rect.size.x <= 0 or view_rect.size.y <= 0:
		return Rect2()
	var scale := minf(view_rect.size.x / texture_size.x, view_rect.size.y / texture_size.y)
	var size := texture_size * scale
	return Rect2(view_rect.position + (view_rect.size - size) * 0.5, size)

static func surface_position(point: Vector2, drawn: Rect2, logical_size: Vector2, grabbed := false) -> Vector2:
	if not point.is_finite() or drawn.size.x <= 0 or drawn.size.y <= 0 or logical_size.x <= 0 or logical_size.y <= 0:
		return Vector2.INF
	if not grabbed and not drawn.has_point(point):
		return Vector2.INF
	return (point - drawn.position) / drawn.size * logical_size

static func evdev(button: int) -> int:
	match button:
		MOUSE_BUTTON_LEFT: return 272
		MOUSE_BUTTON_RIGHT: return 273
		MOUSE_BUTTON_MIDDLE: return 274
		MOUSE_BUTTON_XBUTTON1: return 275
		MOUSE_BUTTON_XBUTTON2: return 276
	return 0

static func wheel(button: int, factor: float) -> Vector2:
	if not is_finite(factor) or factor < 0: return Vector2.ZERO
	match button:
		MOUSE_BUTTON_WHEEL_UP: return Vector2(0, -10 * factor)
		MOUSE_BUTTON_WHEEL_DOWN: return Vector2(0, 10 * factor)
		MOUSE_BUTTON_WHEEL_LEFT: return Vector2(-10 * factor, 0)
		MOUSE_BUTTON_WHEEL_RIGHT: return Vector2(10 * factor, 0)
	return Vector2.ZERO
