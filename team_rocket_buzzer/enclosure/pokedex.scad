// Millimetres. Export base and face separately; layout is their print orientation.
part = "layout"; // [layout,base,face,assembly]
$fn = 64;
width = 82;
height = 128;
depth = 24;
wall = 2.4;
button_diameter = 12.4; // Measure the threaded barrel of your panel switch.
button_x = 60;
button_y = 40;
pcb_thickness = 1.0; // Measure your board; space above the support posts.
pcb_clearance = 0.25;
clip_thickness = 1.2;
lid_clearance = 0.3;
lid_engagement = 0.7;
lid_clip_y = [32,96];
pico_x = 15;
pico_y = 74; // PCB spans y=74..125; USB faces the top edge.

module outline(w, h, r, z) {
    linear_extrude(z) hull()
        for (x = [r, w-r], y = [r, h-r])
            translate([x,y]) circle(r=r);
}

// Extrude an X/Z profile along positive Y.
module xz_prism(points, length) {
    translate([0,length,0]) rotate([90,0,0])
        linear_extrude(length) polygon(points);
}

// Left-side lid hook; mirrored for the right side. Ramp leads during insertion.
module lid_clip() {
    x = wall + lid_clearance;
    z = wall + 7.4;
    xz_prism([[x,wall-0.1], [x+clip_thickness,wall-0.1],
              [x+clip_thickness,wall+10], [x,wall+10],
              [wall-lid_engagement,z+0.6], [wall-lid_engagement,z],
              [x,z]], 8);
}

// Board clips touch only the PCB edges. Upper ramp lets the board press in.
module board_clip() {
    z = wall + 4 + pcb_thickness + pcb_clearance;
    xz_prism([[-0.3-clip_thickness,wall-0.1], [-0.3,wall-0.1],
              [-0.3,z], [0.5,z], [0.5,z+0.4],
              [-0.3,z+1.6], [-0.3-clip_thickness,z+1.6]], 5);
}

module base() {
    difference() {
        union() {
            difference() {
                outline(width,height,7,depth);
                translate([wall,wall,wall])
                    outline(width-2*wall,height-2*wall,4.6,depth);
            }
            for (y=[pico_y+20,pico_y+36]) {
                translate([pico_x,y,0]) board_clip();
                translate([pico_x+21,y,0]) mirror([1,0,0]) board_clip();
            }
            // Low end stops keep the PCB from sliding, clear of the USB plug.
            for (x=[pico_x+2,pico_x+17], y=[pico_y-1.5,pico_y+51.3])
                translate([x,y,wall]) cube([2,1.2,4.8]);
            // 17.78 x 48.26 mm mounting grid, 4 mm space under the PCB.
            for (x=[pico_x+1.61,pico_x+19.39],
                 y=[pico_y+1.37,pico_y+49.63])
                translate([x,y,wall]) cylinder(h=4,r=2.5);
        }
        // Accessible windows: press the hooks inward to release the lid.
        for (y=lid_clip_y, x=[-0.1,width-wall-0.1])
            translate([x,y-0.4,depth-10.4])
                cube([wall+0.2,8.8,3.3]);
        // Generous micro-USB plug opening; verify your cable overmould.
        translate([pico_x+10.5-7,height-wall-1,wall+2])
            cube([14,wall+2,10]);
    }
}

module face() {
    difference() {
        union() {
            outline(width,height,7,wall);
            for (y=lid_clip_y) {
                translate([0,y,0]) lid_clip();
                translate([width,y,0]) mirror([1,0,0]) lid_clip();
            }
            // Short rails leave the hooks free to flex.
            for (x=[wall+0.3,width-wall-1.9], y=[18,48,78,110])
                translate([x,y,wall]) cube([1.6,8,2]);
            for (y=[wall+0.3,height-wall-1.9])
                translate([18,y,wall]) cube([width-36,1.6,2]);
        }
        translate([button_x,button_y,-0.1])
            cylinder(h=wall+0.2,d=button_diameter);
        // Sound slots; mount a piezo behind them using removable foam tape.
        for (y=[68:4:88]) translate([49,y,-0.1]) cube([22,1.6,wall+0.2]);
        // Shallow decorative recesses on the outside (print-bed face).
        translate([10,48,-0.1]) cube([30,39,0.9]);
        translate([19,108,-0.1]) cylinder(h=0.9,d=20);
        for (x=[38,47,56]) translate([x,111,-0.1]) cylinder(h=0.9,d=5);
        translate([15,24,-0.1]) cube([21,6,0.9]);
        translate([22.5,16.5,-0.1]) cube([6,21,0.9]);
    }
}

if (part == "base") base();
else if (part == "face") face();
else if (part == "assembly") {
    color("firebrick") base();
    color("red") translate([0,0,depth+wall]) mirror([0,0,1]) face();
} else if (part == "layout") {
    base();
    translate([width+10,0,0]) face();
} else assert(false, "Unknown part");
