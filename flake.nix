{
  description = "Orus M0 hermetic Nix and Bazel development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

    glaze = {
      url = "github:stephenberry/glaze/v7.5.0";
      flake = false;
    };

    googletest = {
      url = "github:google/googletest/v1.17.0";
      flake = false;
    };

    google_benchmark = {
      url = "github:google/benchmark/v1.9.5";
      flake = false;
    };
  };

  outputs = inputs@{
    self,
    nixpkgs,
    glaze,
    googletest,
    google_benchmark,
    ...
  }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      llvm = pkgs.llvmPackages_21;
      python = pkgs.python314.withPackages (packages: [ packages.coverage ]);
      shellPackages = [
        pkgs.bash
        pkgs.bazel_8
        pkgs.binutils
        pkgs.cacert
        llvm.clang
        llvm.clang-tools
        llvm.lld
        pkgs.coreutils
        pkgs.findutils
        pkgs.gawk
        pkgs.gcc15
        pkgs.gitMinimal
        pkgs.gnugrep
        pkgs.gnused
        pkgs.jq
        pkgs.nixfmt
        pkgs.openssl
        pkgs.pkg-config
        python
        pkgs.utf8proc
      ];
      pinnedPath = pkgs.lib.makeBinPath shellPackages;
      environment = {
        ORUS_BAZEL = "${pkgs.bazel_8}/bin/bazel";
        ORUS_CLANG = "${llvm.clang}/bin/clang";
        ORUS_CLANGXX = "${llvm.clang}/bin/clang++";
        ORUS_GCC = "${pkgs.gcc15}/bin/gcc";
        ORUS_GCC_LIB = "${pkgs.gcc15.cc.lib}";
        ORUS_GXX = "${pkgs.gcc15}/bin/g++";
        ORUS_LLD = "${llvm.lld}/bin/ld.lld";
        ORUS_LLVM = "${llvm.llvm}";
        ORUS_PYTHON = "${python}/bin/python3";
        ORUS_OPENSSL = "${pkgs.openssl.out}";
        ORUS_OPENSSL_DEV = "${pkgs.openssl.dev}";
        ORUS_UTF8PROC = "${pkgs.utf8proc}";
        ORUS_GLAZE_SRC = "${glaze}";
        ORUS_GOOGLETEST_SRC = "${googletest}";
        ORUS_BENCHMARK_SRC = "${google_benchmark}";
        ORUS_NETWORK_ALLOWED = "0";
        LANG = "C.UTF-8";
        LC_ALL = "C.UTF-8";
        SOURCE_DATE_EPOCH = "0";
      };
      exportEnvironment = pkgs.lib.concatStringsSep "\n" (
        pkgs.lib.mapAttrsToList (name: value: "export ${name}=${pkgs.lib.escapeShellArg value}") environment
      );
      bcrManifest = builtins.fromJSON (builtins.readFile ./config/bcr-distdir.json);
      bcrRegistry = pkgs.fetchzip {
        url = bcrManifest.registry.url;
        hash = bcrManifest.registry.integrity;
      };
      rulesCcArchive = builtins.head (
        builtins.filter (archive: builtins.match ".*/rules_cc-0.2.22.tar.gz" archive.url != null) bcrManifest.archives
      );
      rulesCcSource = pkgs.runCommand "orus-rules_cc-0.2.22-source" { } ''
        mkdir "$out"
        tar -xzf ${pkgs.fetchurl {
          inherit (rulesCcArchive) url;
          hash = rulesCcArchive.integrity;
        }} --strip-components=1 -C "$out"
        chmod -R u+w "$out"
        patchShebangs "$out"
      '';
      bcrDistdir = pkgs.linkFarm "orus-m0-bcr-distdir" (
        map (archive: {
          name = builtins.baseNameOf archive.url;
          path = pkgs.fetchurl {
            inherit (archive) url;
            hash = archive.integrity;
          };
        }) bcrManifest.archives
      );
    in
    {
      devShells.${system}.default = pkgs.mkShell (
        environment
        // {
          packages = shellPackages;
          shellHook = ''
            export PATH=${pkgs.lib.escapeShellArg "${self}/toolchains/bin:${pinnedPath}"}
            ${exportEnvironment}
          '';
        }
      );

      formatter.${system} = pkgs.nixfmt;

      checks.${system}.bootstrap-contract = pkgs.runCommand "orus-m0-001-bootstrap-contract" {
        nativeBuildInputs = [ pkgs.bazel_8 python ];
      } ''
        cp -R ${self} source
        chmod -R u+w source
        patchShebangs source/toolchains/bin
        cd source
        ${exportEnvironment}
        export PATH="$PWD/toolchains/bin:${pinnedPath}"
        bazelInstallBase="$(${pkgs.bazel_8}/bin/bazel \
          --output_user_root="$TMPDIR/bazel-root" \
          info install_base)"
        patchShebangs "$bazelInstallBase/embedded_tools/tools/test"
        # The enclosing Nix derivation already has no network namespace. Avoid
        # asking the nested Bazel sandbox to mount the intentionally absent
        # /sys while retaining the same network-denied effective boundary.
        ${pkgs.bazel_8}/bin/bazel --output_user_root="$TMPDIR/bazel-root" test \
          --config=dev \
          --distdir=${bcrDistdir} \
          --lockfile_mode=off \
          --override_module=rules_cc=${rulesCcSource} \
          --registry=file://${bcrRegistry} \
          --noexperimental_split_xml_generation \
          --sandbox_default_allow_network=true \
          //tests/build/...
        ${pkgs.bazel_8}/bin/bazel --output_user_root="$TMPDIR/bazel-root" run \
          --distdir=${bcrDistdir} \
          --lockfile_mode=off \
          --override_module=rules_cc=${rulesCcSource} \
          --registry=file://${bcrRegistry} \
          --sandbox_default_allow_network=true \
          //tools:format
        mkdir -p "$out"
        printf '%s\n' 'M0-001 Bazel bootstrap contract passed' > "$out/result.txt"
      '';
    };
}
