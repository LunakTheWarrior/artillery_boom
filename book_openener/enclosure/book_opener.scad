// Prototype for a 152 x 230 x 10 mm paperback. Millimetres.
// Parts print separately. Flexible fabric-tape hinge, cord winch, weak elastic return.
// No printed rigid hinge: it must accommodate the paperback's bending spine.
part = "motor_mount"; // [back,paddle,foot,spool,guide,motor_mount,uno_tray,sensor_mount]
book_w = 152;
book_h = 230;
book_t = 10;
plate = 3;
paddle_t = 1.6;
lean = 20; // Degrees back from vertical.
back_height = book_h+12;
foot_depth = 110;
foot_height = back_height*cos(lean);
foot_tip_depth = back_height*sin(lean)+plate*cos(lean);
motor_d = 28.5; // ASSUMED 28BYJ-48 body: measure yours.
shaft_d = 5.2; // Clearance diameter, tune a test fit.
shaft_flats = 3.2; // Double-flat shaft width, measure yours.
$fn = 64;

module rounded_plate(w,h,t,r=3) {
    linear_extrude(t) hull()
        for(x=[r,w-r],y=[r,h-r]) translate([x,y]) circle(r=r);
}
module back() {
    difference() {
        union() {
            rounded_plate(book_w+12,book_h+12,plate);
            // Bottom ledge supports the book, but stops short of the spine hinge.
            translate([6,0,-(book_t+5)]) cube([book_w,4,book_t+5]);
        }
        // Reusable ties/ribbon around the stationary back half; avoid visible pages.
        for(x=[9,book_w+3],y=[30,book_h-18])
            translate([x-2,y,-0.1]) cube([4,12,plate+0.2]);
        // Feet align behind these two strips; glue/strong removable mounting tape.
    }
}
module paddle() {
    difference() {
        union() {
            // Leave 3 mm clearance at spine and at page edges.
            rounded_plate(book_w-6,book_h-6,paddle_t);
            translate([book_w-9,book_h/2-10,0])
                rounded_plate(18,20,paddle_t);
        }
        translate([book_w+1,book_h/2,-0.1]) cylinder(h=paddle_t+0.2,d=3);
        // Slots for soft ribbon holding front cover + first half of pages to paddle.
        for(y=[24,book_h-42])
            translate([book_w-14,y,-0.1]) cube([3,12,paddle_t+0.2]);
    }
}
// Print on broad triangular side. In use rotate to stand on the long base edge.
// Front sloping edge supports the rear plate at 20 degrees from vertical.
module foot() {
    linear_extrude(14) polygon([
        [plate*cos(lean),0],
        [foot_depth,0],
        [foot_tip_depth,foot_height]
    ]);
}
module spool() {
    difference() {
        union() {
            cylinder(h=2,d=26);
            translate([0,0,2]) cylinder(h=12,d=16);
            // Chamfer instead of an unsupported top flange.
            translate([0,0,14]) cylinder(h=5,d1=16,d2=26);
            translate([0,0,19]) cylinder(h=1,d=26);
        }
        // Blind double-flat socket from bed face, 10 mm deep.
        translate([0,0,-0.1]) linear_extrude(10.1)
            intersection() {
                circle(d=shaft_d);
                square([shaft_flats,shaft_d+1],center=true);
            }
        // Tie the cord through this hole in the upper flange.
        translate([10,0,12]) cylinder(h=9,d=2);
    }
}
// Tape/bolt this guide securely to the table left and in front of the spine.
// Guide height is adjustable with a separate block if needed.
module guide() {
    difference() {
        union() {
            rounded_plate(40,40,4);
            translate([16,16,4]) cube([8,8,46]);
            translate([20,20,50]) rotate([90,0,0])
                cylinder(h=8,d=14,center=true);
            translate([16,4,4]) rotate([90,0,90])
                linear_extrude(8) polygon([[0,0],[28,0],[14,40]]);
        }
        translate([20,20,50]) rotate([90,0,0])
            cylinder(h=12,d=5,center=true);
        for(x=[6,34],y=[6,34])
            translate([x,y,-0.1]) cylinder(h=4.2,d=4);
    }
}
// Universal cradle: shaft parallel to mounting surface; fix body with two ties.
// Mount near guide on a heavy base. Keep shaft/cable clear of ties.
module motor_mount() {
    difference() {
        union() {
            rounded_plate(54,40,4);
            translate([7,5,4]) cube([40,30,15]);
        }
        translate([27,4,20]) rotate([-90,0,0]) cylinder(h=32,d=motor_d);
        for(y=[9,27]) translate([3,y,-0.1]) cube([48,3,4.8]);
    }
}
// Separate tray for the Uno Rev3 SMD. Push-fit posts follow Arduino's official
// A000073 Eagle board file: 3.2 mm holes at the coordinates below.
// USB-B/barrel-jack edge faces the open left side for cable access.
module uno_tray() {
    tray_w = 96;
    tray_h = 74;
    board_x = 18;
    board_y = 9;
    holes = [[13.97,2.54],[66.04,7.62],[66.04,35.56],[15.24,50.8]];
    difference() {
        union() {
            rounded_plate(tray_w,tray_h,3,2);
            // Keep all four I/O header edges open; the snaps locate the board.
            for (p=holes) {
                x = board_x+p[0];
                y = board_y+p[1];
                translate([x,y,3]) cylinder(h=2,d=5);
                translate([x,y,4.6]) cylinder(h=2,d=2.65);
                // Ramp the snap head into the 3.2 mm board mounting hole.
                translate([x,y,6.2]) cylinder(h=1.5,d1=2.5,d2=3.7);
                translate([x,y,7.7]) cylinder(h=0.35,d=3.5);
            }
        }
        // Split each peg so its two halves can flex through the board hole.
        for (p=holes)
            translate([board_x+p[0]-3,board_y+p[1]-0.4,4.8])
                cube([6,0.8,3.4]);
        // Tie slots attach the tray to the stand's separate heavy base.
        for (x=[7,tray_w-7],y=[7,tray_h-7])
            translate([x-3,y-1.2,-0.1]) cube([6,2.4,3.2]);
    }
}
// Modulino Distance holder: Arduino board is 41 x 25.36 mm, with 32 x 16 mm
// mounting-hole spacing. Its optical face points outward; the middle is open.
// Two U-clips slide over the 3 mm top edge of the fixed backplate.
module sensor_mount() {
    board_x = 6.5;
    board_y = 6.32;
    holes = [[4.5,4.68],[36.5,4.68],[4.5,20.68],[36.5,20.68]];
    difference() {
        union() {
            difference() {
                rounded_plate(54,38,2.4,2);
                translate([7.5,6.5,-0.1]) cube([39,25,2.6]);
            }
            // Split snap pegs align with the sensor board's four Ø3.2 holes.
            for (p=holes) {
                x=board_x+p[0]; y=board_y+p[1];
                // Thin bridges join each post to the frame outside the PCB hole.
                translate([x-0.4,p[1]<12 ? 6.4 : 29.1,1.8])
                    cube([0.8,2.5,0.6]);
                translate([x,y,2.2]) cylinder(h=1.8,d=5);
                translate([x,y,3.7]) cylinder(h=1.7,d=2.65);
                translate([x,y,5.0]) cylinder(h=1.4,d1=2.5,d2=3.7);
                translate([x,y,6.4]) cylinder(h=0.35,d=3.5);
            }
            // The outer clips touch only the fixed panel's top edge.
            for (x=[5,43]) {
                translate([x,0,-0.5]) cube([6,6,0.8]);
                translate([x,-8,-2.1]) cube([6,14,1.7]);
                translate([x,-8,3.5]) cube([6,11,1.7]);
                translate([x,-8,-0.5]) cube([6,1.7,5.7]);
            }
        }
        // Split the snap heads for a little flex as the PCB presses into place.
        for (p=holes)
            translate([board_x+p[0]-3,board_y+p[1]-0.4,4.8])
                cube([6,0.8,2.1]);
    }
}
// Exploded-free assembly preview. The wooden base and book are visual aids;
// only the individual part selectors are printable parts.
module cord(a,b,d=1.5) {
    hull() {
        translate(a) sphere(d=d);
        translate(b) sphere(d=d);
    }
}
module assembly_foot(x) {
    // Map the printed foot's depth/height profile into world Y/Z so its
    // sloping edge follows the leaning back plate and its base stays level.
    translate([x,0,0]) multmatrix([[0,0,1,0],[-1,0,0,0],[0,1,0,0],[0,0,0,1]])
        foot();
}
module assembly() {
    // Heavy base board is a visual aid; use a securely fixed wooden board.
    color([0.48,0.29,0.14]) translate([-35,-120,-14]) cube([292,270,14]);
    assembly_foot(0);
    assembly_foot(book_w-2);
    // Fixed panel is tilted back; the book sits on the front (-Z) face.
    // Local plate Y is height; rotate it upright and lean its top away from
    // the visitor (positive world Y is the book's front).
    translate([0,0,plate*sin(lean)]) rotate([90+lean,0,0]) {
        color([0.24,0.27,0.30]) back();
        color([0.78,0.62,0.40]) translate([6,6,-book_t]) cube([book_w,book_h,book_t]);
        // Open half of the book on its flexible tape hinge.
        color([0.92,0.88,0.74])
            translate([6,6,-book_t-1]) rotate([0,48,0]) translate([-6,0,0]) paddle();
        // Top-mounted sensor board, aimed out toward the approaching visitor.
        translate([55,back_height,3]) mirror([0,0,1]) sensor_mount();
        color([0.12,0.55,0.66])
            translate([55+6.5,back_height+6.32,-4.5]) cube([41,25.36,1.6]);
    }
    // The eye faces along the cord path: paddle tab -> guide -> winch spool.
    // Both mounting plates sit fully on the heavy base board.
    color([0.82,0.24,0.18]) translate([190,90,0]) rotate([0,0,90]) guide();
    color([0.25,0.29,0.34]) translate([193,46,0]) motor_mount();
    // Output shaft faces forward; the body ends at y=78 and the spool starts there.
    color([0.42,0.44,0.47]) translate([220,50,20]) rotate([-90,0,0]) cylinder(h=28,d=motor_d);
    color([0.86,0.66,0.18]) translate([220,78,20]) rotate([-90,0,0]) spool();
    // Uno tray with a simple board silhouette; USB edge remains accessible.
    color([0.30,0.33,0.36]) translate([28,-107,0]) uno_tray();
    color([0.05,0.40,0.24]) translate([46,-98,5]) cube([68.58,53.34,1.6]);
    color([0.12,0.12,0.14]) translate([46,-98,6.6]) cube([16,15,7]);
    color([0.12,0.12,0.14]) translate([109,-92,6.6]) cube([5,16,10]);
    // Red cord meets the open paddle tab, passes through the eye, and winds
    // around the spool. The guide stays outside the book's page edge.
    cord([104,71,156],[170,110,50],1.4);
    cord([170,110,50],[230,90,20],1.4);
}
if(part=="back") back();
else if(part=="paddle") paddle();
else if(part=="foot") foot();
else if(part=="spool") spool();
else if(part=="guide") guide();
else if(part=="motor_mount") motor_mount();
else if(part=="uno_tray") uno_tray();
else if(part=="sensor_mount") sensor_mount();
else if(part=="assembly") assembly();
else assert(false,"Unknown part");
