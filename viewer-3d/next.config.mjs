import { dirname } from "node:path";
import { fileURLToPath } from "node:url";

const currentDir = dirname(fileURLToPath(import.meta.url));
const workspaceRoot = dirname(currentDir);

/** @type {import('next').NextConfig} */
const nextConfig = {
  turbopack: {
    root: workspaceRoot
  },
  experimental: {
    optimizePackageImports: ["@react-three/drei", "@react-three/fiber", "three"]
  },
  images: {
    unoptimized: true
  }
};

export default nextConfig;
