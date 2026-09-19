# Mansion Desktop

## Concept and Architecture

Mansion Desktop is a proposed Linux desktop environment that replaces the traditional two-dimensional desktop metaphor with a persistent, navigable three-dimensional world.

The user begins inside a large mansion and moves through it much like a first-person game, using a conventional monitor, keyboard, and mouse. Rooms, desks, shelves, walls, drawers, and other locations provide spatial organization for applications, documents, projects, and running sessions.

The central idea is:

> Application and file identity should be associated with persistent physical location, not merely window coordinates, virtual desktops, or directory paths.

The mansion is not just a decorative file browser. It is the desktop shell itself. Ordinary Linux applications continue to run through Wayland, while the compositor presents their surfaces as objects in the 3D environment.

---

## 1. Core Experience

### 1.1 The mansion is the desktop

At login, the user appears inside a mansion rather than on a conventional desktop. The mansion might include:

- A large central hall
- Offices and studies
- Libraries and bookshelves
- Project rooms
- Server rooms
- Tables, desks, drawers, cabinets, and walls
- Staircases, corridors, doors, and portals

These spaces provide memorable physical locations for digital work. Instead of remembering that a terminal was on virtual desktop four, the user might remember that it is on the console in the downstairs server room.

### 1.2 Applications are persistent physical artifacts

An application can exist as an object in the world: a monitor on a desk, a laptop, a book, a framed panel, a control console, or another representation appropriate to its role.

When the user interacts with an application artifact:

1. Its live application surface becomes fullscreen.
2. The user interacts with it as an ordinary desktop application.
3. When the user leaves that view, the application returns to its in-world representation.
4. The application can remain running even though it is no longer fullscreen.

From the application's perspective, it remains a normal Wayland application. The compositor controls how and where its surface is shown.

### 1.3 Files are physical artifacts

Files also have persistent representations in the mansion. A PDF might appear as a document on a desk, source code as a notebook or project object, an image as a framed picture, or a folder as a container.

The 3D location is metadata layered on top of the ordinary Linux filesystem. For example:

```text
Filesystem:
  /home/chris/Documents/tax-return.pdf

Spatial location:
  Second Floor / Study / Desk / Left Drawer
```

Moving the artifact within the mansion does not need to move the underlying file. This preserves compatibility with shells, scripts, Git, backups, Samba, and other conventional tools.

### 1.4 Opening files with a weapon wheel

When the user interacts directly with a file, Mansion Desktop can display a radial, weapon-wheel-style application chooser.

For a PDF, the choices might include:

- Okular
- Firefox
- LibreOffice
- VS Code

The available applications can come from standard Linux desktop entries, MIME associations, and XDG application-selection mechanisms. Choosing an application launches or activates it at the file's current location.

### 1.5 Saving becomes placing an artifact

Saving a new document can become a spatial action rather than only a pathname-based dialog:

1. The application requests a save location.
2. Mansion Desktop returns the user to the world carrying a new artifact.
3. The user places it on a desk, shelf, wall, or in a drawer.
4. The file is saved to a normal filesystem path.
5. Its spatial placement is recorded separately.

This behavior could eventually be implemented through a custom XDG Desktop Portal file-chooser backend. Applications that do not use portals would need a conventional fallback.

---

## 2. Design Principles

### 2.1 Preserve ordinary Linux compatibility

The system should not replace the Linux filesystem with a simulated physical hierarchy. Spatial organization should be an additional metadata layer.

This allows users and applications to continue using:

- Normal filesystem paths
- Command-line tools
- Git repositories
- Backup and synchronization tools
- Network shares
- Existing application save and open behavior

### 2.2 Applications should not need Mansion-specific support

The compositor should make ordinary Wayland applications usable without requiring them to understand the mansion metaphor.

An application should see an ordinary Wayland display, receive ordinary input events, and render ordinary windows. Mansion Desktop decides whether the resulting surface is fullscreen or mapped onto an object in the world.

### 2.3 Use spatial memory without becoming trapped by realism

The mansion should provide locality and memory cues, but it should not force users to imitate every inconvenience of physical space.

Useful nonphysical features could include:

- Instant teleportation to rooms or favorite objects
- Search that creates a visible trail to an artifact
- Rooms that are larger inside than outside
- Portals to remote systems or other workspaces
- Frequently used items appearing nearby
- Global shortcuts for immediate application access
- An inventory or quick-access wheel

The physical metaphor should organize work, not slow it down.

### 2.4 The world is persistent

Locations should survive application focus changes and system restarts. The user should be able to return to a room and find its artifacts where they were left.

The system can reliably restore placement, file associations, launch commands, and document URIs. It cannot universally restore arbitrary in-memory application state. Exact restoration of tabs, cursor positions, scroll state, or unsaved content depends on each application's own session support.

---

## 3. System Architecture

Mansion Desktop is best understood as a Wayland compositor and desktop shell whose primary user interface is a 3D engine.

```text
Linux applications
Firefox / Terminal / VS Code / LibreOffice
                |
        Wayland and XWayland
                |
       Mansion compositor
 window management, input, focus,
 app lifecycle, surface management
                |
       live application surfaces
                |
         3D world renderer
 mansion, objects, lighting, physics,
 interaction, transitions, shell UI
                |
          Vulkan / GPU
```

The compositor receives application buffers and can either:

- Render a selected application directly as a fullscreen surface, or
- Use that surface as a live texture on an object in the 3D scene.

### 3.1 Suggested modules

The project can be divided conceptually into four major components.

#### `mansion-compositor`

- Wayland server
- Surface and window lifecycle
- Input routing and focus
- Application spawning and activation
- Fullscreen and in-world transitions
- XWayland integration
- Clipboard and drag-and-drop integration

#### `mansion-world`

- 3D rendering
- Mansion geometry
- Camera and movement
- Lighting and animation
- Physics and ray casting
- In-world object interaction
- Mapping application surfaces onto scene geometry

#### `mansion-shell`

- Application launcher
- Weapon-wheel application chooser
- Search and teleportation
- Notifications
- Task switching
- Global shortcuts
- Inventory or quick-access interface

#### `mansion-indexer`

- Filesystem observation and indexing
- MIME-type detection
- Artifact metadata
- Thumbnails and previews
- Search
- Persistent spatial database

These can begin as modules within one executable and be separated into processes only when useful.

### 3.2 Spatial metadata

A small database such as SQLite can map conventional resources to objects in the world.

An artifact record might contain:

```text
uuid
resource_uri
artifact_type
world_position
world_orientation
room_id
container_id
representation
preferred_application
last_application
launch_metadata
created_at
updated_at
```

Application instances need similar records connecting an in-world object to a live Wayland surface and, when possible, to enough launch information to recreate the session later.

### 3.3 Possible implementation stacks

Two plausible native approaches are:

#### C++ approach

- C++20 or C++23
- wlroots
- Vulkan
- GLM
- Jolt or Bullet for physics
- SQLite

#### Rust approach

- Rust
- Smithay
- Vulkan or wgpu
- Rapier
- SQLite

Smithay is attractive because it provides building blocks for custom Wayland compositors and includes the Anvil sample compositor. For a developer already comfortable in C++, wlroots plus Vulkan may provide a faster route to experimentation.

A traditional game engine could be useful for a visual mockup, but the final system's most difficult responsibilities are display-server concerns: Wayland protocols, surface trees, DMA-BUF, input, focus, XWayland, DRM/KMS, session management, and portals. The core architecture should therefore be compositor-first rather than built around Unity, Unreal, or Godot.

---

## 4. Key Interaction Mechanics

### 4.1 Application focus modes

The system has at least two primary input modes:

#### World mode

- Mouse movement controls the camera.
- Keyboard input controls movement and world actions.
- A ray cast determines which artifact the user is targeting.
- An interaction key selects, opens, moves, or inspects the artifact.

#### Application mode

- Keyboard focus belongs to the selected application.
- Pointer motion and clicks are delivered to its Wayland surface.
- The application is fullscreen or otherwise presented for direct use.
- A reserved system key returns control to world mode.

The transition should be immediate and should not terminate or suspend the application unless explicitly requested.

### 4.2 Interacting with an embedded live surface

If an application is usable while visible on an in-world monitor, the compositor must translate a ray intersection into coordinates within the Wayland surface:

```text
camera ray -> object intersection -> texture coordinates -> surface x/y
```

Those coordinates become Wayland pointer events. Keyboard focus must transfer to the application when appropriate and return to the world cleanly.

### 4.3 Moving artifacts

An artifact may be picked up and placed on another valid surface or inside a container. This changes the artifact's spatial metadata without necessarily changing its underlying file path or restarting its application.

Possible placement rules include:

- Documents can lie on desks or go into drawers.
- Images can be placed on walls.
- Applications can occupy monitors, consoles, or floating panels.
- Project containers can hold related files and application launchers.
- Special artifacts can act as doors or portals.

The rules should help recognition but remain flexible rather than rigidly simulate reality.

---

## 5. Technical Challenges

### 5.1 Input routing

The compositor must decide whether input belongs to the world or an application. Embedded surfaces require accurate coordinate transforms, button events, scrolling, pointer constraints, text input, and keyboard focus.

This is one of the project's central technical problems.

### 5.2 Surface trees and transient windows

Applications often create several related Wayland surfaces:

- Main windows
- Menus
- Dialogs
- Tooltips
- Context menus
- Subsurfaces

These must remain spatially and behaviorally connected. A dialog belonging to an application should not accidentally appear as an unrelated object elsewhere in the mansion.

### 5.3 GPU buffer integration

The compositor must efficiently import and render application buffers in the 3D scene. A mature implementation will need to handle topics such as:

- DMA-BUF import
- Explicit synchronization
- Vulkan resource ownership
- Frame callbacks and presentation timing
- Hardware cursors or software cursor composition
- NVIDIA driver behavior

The first prototype can avoid some optimization work, but the final desktop cannot rely on expensive CPU copies for every application frame.

### 5.4 X11 compatibility

XWayland support will be needed for older or X11-only applications. Its windows should participate in the same artifact and focus model as native Wayland applications.

### 5.5 File chooser integration

Applications that use XDG Desktop Portals can eventually receive a Mansion-native open/save experience. Applications with custom or toolkit-native file choosers may initially continue to show ordinary dialogs.

### 5.6 Session restoration

Mansion Desktop can restore:

- Artifact positions
- Associated file paths or URIs
- Application launch commands
- Room and container membership
- Preferred representations

It cannot guarantee restoration of every application's internal state. The system should cooperate with application-native session restoration when available and otherwise relaunch the application with the associated document or URI.

### 5.7 Accessibility and usability

A first-person interface creates risks that ordinary desktops do not have:

- Motion sickness
- Excessive travel time
- Difficulty locating forgotten objects
- Motor accessibility concerns
- High interaction cost for routine tasks

Search, teleportation, global shortcuts, adjustable movement, optional reduced-motion transitions, conventional list views, and keyboard-only navigation should be considered fundamental features rather than late additions.

---

## 6. Example Organization

The mansion can reflect projects and areas of responsibility.

```text
Mansion
|- Account Processor room
|  |- VS Code workstation
|  |- Architecture document on table
|  |- Kafka terminal console
|  `- TODO whiteboard
|- Market Data room
|  |- Quote monitor wall
|  `- Documentation browser
|- LunchTrucker room
|  |- Planning desk
|  |- Browser research board
|  `- Design assets shelf
`- Personal room
   |- Documents cabinet
   `- Media area
```

An SSH connection could appear as a terminal in a server room. A door or portal could represent another machine. A project room could preserve an entire working context: editor, terminals, browser documentation, diagrams, and relevant files.

---

## 7. Development Strategy

The project should begin as a nested compositor running inside an existing desktop session. Applications launched into its Wayland display appear inside the prototype window.

This allows development of the novel parts before Mansion Desktop must control physical displays, input devices, virtual terminals, or Linux sessions directly.

### Milestone 1: Prove the core interaction

Build one simple room containing one desk and one application surface.

The exact proof should be:

1. Start Mansion Desktop as a nested compositor.
2. Walk around a basic 3D room.
3. Launch one native Wayland terminal.
4. Display its live surface on a monitor on the desk.
5. Walk up and select it.
6. Switch it to fullscreen application mode.
7. Type and click inside it normally.
8. Press a reserved key to return to the room.
9. Confirm the terminal is still running on the desk.

This proves most of the technically novel interaction: application surface capture, 3D presentation, focus transfer, input routing, and fullscreen/in-world transitions.

The first room should use simple geometry, simple lighting, and a modest resolution. Visual polish is not part of this proof.

### Milestone 2: Multiple applications and persistence

- Launch several Wayland clients.
- Give each one an in-world representation.
- Move application artifacts between valid locations.
- Save their transforms and room membership.
- Restore the world layout after restarting Mansion Desktop.
- Add basic task switching and search.

### Milestone 3: File artifacts

- Index selected filesystem locations.
- Create artifact representations for files.
- Open files using MIME associations.
- Add the radial application chooser.
- Persist file placement separately from filesystem paths.
- Add thumbnails and recognizable representations.

### Milestone 4: Desktop integration

- Implement XWayland support.
- Add clipboard and drag-and-drop behavior.
- Add notifications.
- Implement a Mansion-native XDG portal backend for open/save workflows.
- Improve session recreation.
- Add audio and richer shell controls.

### Milestone 5: Full mansion

- Multiple rooms and floors
- Project-oriented spaces
- Doors, portals, search trails, and teleportation
- Customizable architecture and furniture
- Better physics, lighting, animation, and visual identity
- Scalable indexing and artifact management

### Milestone 6: Native desktop session

Only after the nested version is mature should Mansion Desktop run directly as the desktop session. This phase adds:

- DRM/KMS display control
- GBM and direct GPU integration
- libinput
- TTY and seat/session management
- Monitor discovery and modesetting
- Login/session integration
- Robust failure recovery

---

## 8. Non-Goals for the First Prototype

The initial prototype should not attempt to provide:

- A complete mansion
- Photorealistic graphics
- A replacement filesystem
- Perfect session restoration
- A complete XDG portal implementation
- XWayland compatibility
- Multiple monitors
- Direct boot as the system desktop
- VR support
- Remote-machine portals
- Complex physics

These are valuable future capabilities, but they would obscure the first essential experiment.

---

## 9. Open Design Questions

Several decisions can remain open until the core prototype works:

- Should application surfaces always become fullscreen, or can some be operated directly in the world?
- How should multiple windows belonging to one application be represented?
- What happens when a file is moved, renamed, or deleted outside Mansion Desktop?
- Should room placement be per-user, per-machine, or synchronized?
- How are artifacts created for files that have never been explicitly placed?
- How should very large collections be represented without filling rooms with clutter?
- Can a room act as both a spatial workspace and a conventional searchable collection?
- How should unsaved documents appear and be recovered?
- Which interactions must always have a fast non-spatial alternative?
- How much environmental customization should be supported?

The prototype should collect evidence for these decisions instead of settling them all in advance.

---

## 10. Definition of Success

Mansion Desktop succeeds if physical locality provides a genuinely useful way to remember and resume work—not merely an impressive visual effect.

The project should make it natural to think:

> "The production terminal is downstairs in the server room, and the design document is on the project-room table."

rather than:

> "Which virtual desktop, tab, directory, or window did I leave that in?"

The smallest meaningful validation is therefore not a beautiful mansion. It is a single room in which an ordinary Linux application becomes a persistent, interactive object and can move cleanly between the 3D world and focused desktop use.
