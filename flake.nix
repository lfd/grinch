{
  description = "Grinch -- a minimalist OS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
        # Single source of truth for the GCC series used by both the
        # host stdenv and the riscv cross compilers. Keeping them in
        # lockstep matters for gcov: gcov_extract is linked against
        # libgcov from the host gcc and reads .gcda files produced by
        # the cross gcc, so a version mismatch trips libgcov's stamp
        # check.
        gccVersion = "gcc16";
        hostStdenv = pkgs."${gccVersion}Stdenv";
        crossTools = arch:
          let p = pkgs.pkgsCross.${arch}.buildPackages;
          in [ p.gdb p.${gccVersion} ];
      in
      {
        devShell = (pkgs.mkShell.override { stdenv = hostStdenv; }) {
          NIX_HARDENING_ENABLE="";
          buildInputs =
            (crossTools "riscv32") ++
            (crossTools "riscv64") ++
            (with pkgs; [
            # Tools required for grinch
            hostname
            cpio
            qemu

            # Image manipulation
            python313
            python313Packages.pillow

            # Required for u-boot
            bison
            flex
            openssl
            gnutls
            ncurses
            pkg-config
            xxd

            # GCOV report generation (lcov + genhtml)
            lcov

            # Packages for development
            less
            git
            which
          ]);
        };
      }
    );
}
