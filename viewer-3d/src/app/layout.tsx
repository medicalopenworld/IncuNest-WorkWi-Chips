import type { Metadata } from "next";
import { Chakra_Petch, Spectral } from "next/font/google";

import "./globals.css";

const displayFont = Chakra_Petch({
  variable: "--font-display",
  weight: ["500", "600", "700"],
  subsets: ["latin"]
});

const bodyFont = Spectral({
  variable: "--font-body",
  weight: ["400", "500", "600"],
  subsets: ["latin"]
});

export const metadata: Metadata = {
  title: "IncuNest Digital Twin",
  description: "3D and telemetry viewer for the virtual IncuNest incubator simulator."
};

export default function RootLayout({
  children
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="es">
      <body suppressHydrationWarning className={`${displayFont.variable} ${bodyFont.variable}`}>
        {children}
      </body>
    </html>
  );
}
