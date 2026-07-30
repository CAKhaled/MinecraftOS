with open('textured_cube.c', 'r') as f:
    content = f.read()

start = content.find('void render_textured_cube')
end = content.find('void rotateCube', start)

new_func = """void render_textured_cube(uint8_t angle_x, uint8_t angle_y, uint8_t angle_z) {
    clear_screen_textured();

    int sinX = get_sin(angle_x), cosX = get_cos(angle_x);
    int sinY = get_sin(angle_y), cosY = get_cos(angle_y);
    int sinZ = get_sin(angle_z), cosZ = get_cos(angle_z);

    Vector3 proj_3d[8];
    Vertex2D screen[8];

    // تدوير وتكبير وإسقاط الرؤوس الثمانية
    for (int i = 0; i < 8; i++) {
        int x = cube_vertices[i].x;
        int y = cube_vertices[i].y;
        int z = cube_vertices[i].z;

        // دوران حول X
        int xy = (y * cosX - z * sinX) / 256;
        int xz = (y * sinX + z * cosX) / 256;
        y = xy; z = xz;

        // دوران حول Y
        int yx = (x * cosY + z * sinY) / 256;
        int yz = (-x * sinY + z * cosY) / 256;
        x = yx; z = yz;

        // دوران حول Z
        int zx = (x * cosZ - y * sinZ) / 256;
        int zy = (x * sinZ + y * cosZ) / 256;
        x = zx; y = zy;

        // حفظ الإحداثيات قبل الإسقاط للتحقق من الاتجاه (Backface Culling)
        proj_3d[i].x = x;
        proj_3d[i].y = y;
        proj_3d[i].z = z;

        // إزاحة Z ليكون المكعب أمام الكاميرا
        z += 150; 
        
        // إسقاط حقيقي (320x200)
        screen[i].x = (x * 256 / z) + (WIDTH / 2);
        screen[i].y = (y * 256 / z) + (HEIGHT / 2); 
    }

    // رسم الأوجه المتوفرة
    for (int i = 0; i < 6; i++) {
        // الرؤوس المعروضة على الشاشة لهذا الوجه
        Vertex2D sv0 = screen[faces[i].v[0]]; sv0.u = 0;  sv0.v = 0;
        Vertex2D sv1 = screen[faces[i].v[1]]; sv1.u = 63; sv1.v = 0;
        Vertex2D sv2 = screen[faces[i].v[2]]; sv2.u = 63; sv2.v = 63;
        Vertex2D sv3 = screen[faces[i].v[3]]; sv3.u = 0;  sv3.v = 63;

        // حساب المتجه العمودي باستخدام إحداثيات الشاشة ثنائية الأبعاد (2D Perspective)
        // هذا يصلح مشكلة التداخل وإخفاء الأوجه الخاطئة
        int dx1 = sv1.x - sv0.x; int dy1 = sv1.y - sv0.y;
        int dx2 = sv2.x - sv0.x; int dy2 = sv2.y - sv0.y;
        
        // مساحة التقاطع الموجهة (Cross Product)
        int normalZ = (dx1 * dy2) - (dy1 * dx2);
        
        // إذا كان المتجه العمودي سالب أو صفر، فهذا يعني أن الوجه ملتف للخلف (مخفي)
        if (normalZ <= 0) continue; 

        const uint8_t* current_texture = faces[i].is_top_bottom ? tex_top_bottom : tex_sides;

        // رسم الوجه كمثلثين
        draw_textured_triangle(sv0, sv1, sv2, current_texture);
        draw_textured_triangle(sv0, sv2, sv3, current_texture);
    }

    flush_buffer();
}
"""

with open('textured_cube.c', 'w') as f:
    f.write(content[:start] + new_func + content[end:])
