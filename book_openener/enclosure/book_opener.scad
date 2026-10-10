// Prototype for a 152 x 230 x 10 mm paperback. Millimetres.
// Parts print separately. Flexible fabric-tape hinge, cord winch, weak elastic return.
// No printed rigid hinge: it must accommodate the paperback's bending spine.
part = "guide"; // [back,paddle,foot,spool,guide,motor_mount]
book_w = 152;
book_h = 230;
book_t = 10;
plate = 3;
paddle_t = 1.6;
lean = 20; // Degrees back from vertical.
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
            translate([16,0,plate]) cube([book_w-10,4,book_t+5]);
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
    linear_extrude(14) polygon([[0,0],[110,0],[130*tan(lean),130]]);
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
if(part=="back") back();
else if(part=="paddle") paddle();
else if(part=="foot") foot();
else if(part=="spool") spool();
else if(part=="guide") guide();
else if(part=="motor_mount") motor_mount();
else assert(false,"Unknown part");
