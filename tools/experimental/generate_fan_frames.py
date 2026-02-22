import math
from PIL import Image, ImageDraw

def generate_svg():
    svg = """<svg width="64" height="64" viewBox="0 0 64 64" xmlns="http://www.w3.org/2000/svg">
  <rect width="64" height="64" fill="#0A1830" />
  <circle cx="32" cy="32" r="30" fill="none" stroke="#404040" stroke-width="3" />
  <g id="fan">
"""
    num_blades = 7
    for i in range(num_blades):
        angle = i * (360 / num_blades)
        # A curved blade path
        svg += f'    <path d="M 32 32 Q 45 15 32 4 Q 20 15 32 32" fill="#EEEEEE" transform="rotate({angle} 32 32)" />\n'
    
    svg += """    <circle cx="32" cy="32" r="8" fill="#606060" />
    <circle cx="32" cy="32" r="4" fill="#505050" />
  </g>
</svg>"""
    with open("chips/incu-fan-pwm/fan.svg", "w") as f:
        f.write(svg)

def draw_fan_frame(angle_deg):
    # Draw at 4x resolution for anti-aliasing
    img = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    cx, cy = 128, 128
    num_blades = 7
    
    for i in range(num_blades):
        base_angle = math.radians(angle_deg + i * (360 / num_blades))
        points = []
        # Create a curved blade polygon
        for r in range(20, 115, 10):
            curve = base_angle - (r - 20) * 0.006
            w = 10 + (r - 20) * 0.35
            x = cx + r * math.cos(curve) - w * math.sin(curve)
            y = cy + r * math.sin(curve) + w * math.cos(curve)
            points.append((x, y))
        for r in range(105, 15, -10):
            curve = base_angle - (r - 20) * 0.006
            w = 10 + (r - 20) * 0.35
            x = cx + r * math.cos(curve) + w * math.sin(curve)
            y = cy + r * math.sin(curve) - w * math.cos(curve)
            points.append((x, y))
            
        draw.polygon(points, fill=(238, 238, 238, 255))
        
    # Hub
    draw.ellipse((cx - 32, cy - 32, cx + 32, cy + 32), fill=(96, 96, 96, 255))
    draw.ellipse((cx - 16, cy - 16, cx + 16, cy + 16), fill=(80, 80, 80, 255))
    
    # Resize to 64x64
    img = img.resize((64, 64), Image.Resampling.LANCZOS)
    
    # Background and frame
    # 0xFF30180A -> A=255, B=48, G=24, R=10
    final_img = Image.new('RGBA', (64, 64), (10, 24, 48, 255))
    final_draw = ImageDraw.Draw(final_img)
    
    # Outer frame
    final_draw.ellipse((2, 2, 61, 61), outline=(64, 64, 64, 255), width=3)
    
    # Composite
    final_img.alpha_composite(img)
    return final_img

def generate_c_header():
    num_frames = 8
    num_blades = 7
    header = "#pragma once\n\n#include <stdint.h>\n\n"
    header += f"#define FAN_NUM_FRAMES {num_frames}\n"
    header += "static const uint32_t fan_frames[FAN_NUM_FRAMES][64 * 64] = {\n"
    
    for f in range(num_frames):
        # 8 frames to cover the angle between two blades (360/7 degrees)
        angle = f * (360.0 / num_blades / num_frames)
        
        img = draw_fan_frame(angle)
        pixels = img.load()
        
        header += "  {\n    "
        for y in range(64):
            for x in range(64):
                r, g, b, a = pixels[x, y]
                # Little Endian 0xAABBGGRR
                val = (a << 24) | (b << 16) | (g << 8) | r
                header += f"0x{val:08X}, "
            header += "\n    "
        header += "},\n"
        
    header += "};\n"
    
    with open("chips/incu-fan-pwm/fan_frames.h", "w") as f:
        f.write(header)

if __name__ == "__main__":
    generate_svg()
    generate_c_header()
    print("Generated fan.svg and fan_frames.h")
