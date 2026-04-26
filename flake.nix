{
  description = "Grinch -- a minimalist OS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      {
        devShell = pkgs.mkShell {
          NIX_HARDENING_ENABLE="";
          buildInputs = with pkgs; [
            # Main debuggers
            pkgsCross.riscv32.buildPackages.gdb
            pkgsCross.riscv64.buildPackages.gdb

            # Works with both. Choose your weapon.
            pkgsCross.riscv32.buildPackages.gcc14
            pkgsCross.riscv64.buildPackages.gcc14
            #pkgsCross.riscv32.buildPackages.gcc15
            #pkgsCross.riscv64.buildPackages.gcc15

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

            # Packages for development
            less
            git
            which
          ];
        };
      }
    );
}
