//!qjs

function draw_mandel(cols, rows, MinIm, MaxIm, MinRe, MaxRe, maxiter) {
  let StepX = (MaxRe - MinRe) / cols;
  let StepY = (MaxIm - MinIm) / rows;
  let Re = MinRe;
  let pal = ' .,:;/=+&%';
  for (let y = 0; y < rows; ++y) {
    let Im = MinIm;
    for (let x = 0; x < cols; ++x) {
      let Zr = Re;
      let Zi = Im;
      let n = 0;
      for (; n < maxiter; ++n) {
        let a = Zr * Zr;
        let b = Zi * Zi;
        if (a + b > 4.0)
          break;
        Zi = 2 * Zr * Zi + Im;
        Zr = a - b + Re;
      }
      std.puts(pal[Math.floor(n * 9 / (maxiter - 1))]);
      Im += StepX;
    }
    std.puts("\n");
    Re += StepY;
  }
}

function animated(t, start, end) { return start * (1.0 - t) + end * t; }

function vt_show_cursor() { std.puts("\x1b[?25h"); }
function vt_hide_cursor() { std.puts("\x1b[?25l"); }
function vt_home() { std.puts("\x1b[H"); }

vt_hide_cursor();

let i00 = -2.0, i10 = -0.105, i01 = 1.5, i11 = 0.105, r00 = -2.5, r10 = -1.5,
    r01 = 1.5, r11 = -1.25, f0 = 30, f1 = 60;

let i;
for (i = 0; i < 100; i++) {
  let t = i / 100.0;
  vt_home();
  draw_mandel(22, 11, animated(t, i00, i10), animated(t, i01, i11),
              animated(t, r00, r10), animated(t, r01, r11),
              animated(t, f0, f1));
}

for (i = 0; i < 100; i++) {
  let t = 1.0 - i / 100.0;
  vt_home();
  draw_mandel(22, 11, animated(t, i00, i10), animated(t, i01, i11),
              animated(t, r00, r10), animated(t, r01, r11),
              animated(t, f0, f1));
}

vt_show_cursor();
