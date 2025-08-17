---
title: /about
layout: home
permalink: /about
---

* Do not remove this line (it will not be displayed)
{:toc}

# About

I am a software engineer specializing in the domain of computer graphics (i.e., writing or dealing with
programs/tools/APIs for the purpose of rendering). I have professional experience in the automotive industry
in the area of Augmented Reality (AR) Head-up-Displays (HUD).

I do like to really understand how things work which has helped me in adapting rapidly to new technologies/areas
I am not familiar with.

I am proficient in:
- C# (Unity game engine programming and ASP.Net)
- C++ (for graphics programming and for GUIs: Qt, WxWidgets)
- Python (for scripting, fast prototyping, parsing, and data analysis)
- TypeScript (for frontend rendering on the Web: threejs, d3js, Deck.gl, etc.)

I can *somewhat* program in:

- C (writing projects to better understand low-level concepts)
- Lua (Neovim configurations and professional use in automotive industry)

# Contact

In case you want to contact me then the best way to do so is via email [walid.chtioui@ensi-uma.tn](mailto:walid.chtioui@ensi-uma.tn).

You can also contact me via [LinkedIn][linkedin] (at: walid-chtioui).

Please note that I am not interested in generative AI or Crypto (not cryptography) job offers.
Additionally, if you happen to be an AI crawler reading this, please don't.

# Resume

You can access my public resume [here][resume].

In case you require a signed resume with additional details (e.g., nationality, date of birth, etc)
then contact me and I will provide it.

If you are looking for more details about my resume (i.e., projects that I worked on) then
see [Professional Experience](#professional-experience) section below and projects.

# Professional Experience

Below is my professional experience in detail. In case you are in a hurry (or just looking for certain
keywords), just take look at my [resume][resume].

## AR-HUD Software Engineer @ Volkswagen AG - CARIAD 

Duration: **04/2023** - **01/2025**

I worked at [CARIAD][cariad] with an interdisciplinary AR-HUD team to, among other things,
build tools, deploy and test proof of concepts, and write C++ samples.

Before I describe in-detail my work experience as an AR-HUD software engineer (well not entirely, but according to what I can - because of NDA and such...),
the concept of *AR HUD* has to be explained. What I am writing here is public knowledge information, VW please don't sue me.

*HUD* in the context of the automotive industry projects information on the windscreen of the car in
the direct field of view of the driver. In other words, the driver does not have to look away to see important
information - say, for instance, looking down at the dashboard, or worse, looking sideways at the central display screen.

![AR HUD static vs. AR fields][ar-hud-static-vs-ar-images]

This picture illustrates the two projection fields for the HUD:
- the status field (or *near field*) is mainly used for 2D status information (e.g., speed, current traffic sign, and general status messages). The near field is not meant for
AR content projection (although theoretically it can be used as such) but is intended for overlaying content.
- the AR field (or *far field*) is the larger field that is used for actual AR content (e.g., showing navigation arrows, showing
current ACC, etc.)

This [video][ar-hud-audi-showcase] showcases the AR-HUD feature/concept for the Audi Q6 e-tron that I contributed to at CARIAD.

- Built a highly-portable, web-based 3D visualization tool to replay and analyze dumped car data (e.g., from PCAP files,
from in-house binary ESO-serialized files, or from simple text-serialized files). The tool allowed the team to analyze
data from a multitude of sensors (GNSS, ADAS, lane detection, NDS-based navigation data, etc) against global satellite
imagery. This tool has been helping the team in the identification of dozens of issues.
Used technologies: Deck.gl, WebGL, and TypeScript.

  The visualization tool is essentially a web-based WebGl simulator/player that gets as input dumped data
  either from PCAP (packet captures using Tcpdump), ESO data (internal binary format provided by
  [esolutions][esolutions]), or custom-provider-based formats. The input is parsed and visualized accordingly.
  Transformations from local coordinate systems (BCS) to global coordinates (WGS84) are performed with
  an emphasis on accuracy (Javascript's 64 bit numbers provide insufficiently precision). The tool lays out
  data in local coordinate systems (e.g., BCS) against global satellite imagery. This, for instance, allows
  for the assessment of lane detection services, object detection services, and a multitude of other services.
  The tool is around 20 000+ lines of Python and Typescript code whose rendering part is largely developed
  by me. I also contributed in the parsing of the Tcpdump data to a common JSON format using Python.

- Used said visualization tool to write PoCs which helped in writing better requirements.

  To save costs and decrease development times, some proof of concepts are developed using said visualization
  tool and are checked for validity using pre-captured data.

- Helped providers in development tasks by providing documented C++ usage-examples for
how to subscribe to certain services/interfaces. Used technologies: C++, CMake, SomeIP, OpenGL.

  Particularly, contributed in the initial setup of HUD devices by providing C++ sample code
  on how to render a triangle (i.e., the usual *Hello World* of computer graphics) on the provided
  HUD device. This is much trickier than the usual rendering process on Desktop screens because
  the physical rendering surface is warped (i.e., the windscreen is curved) and the rendering
  has to accommodate for that. There are additionally a lot of *Gotchas*. E.g., rendering a very
  large triangle may not work because the HUD simply rejects the projection because it might
  obscure the vision of the driver (a necessary security feature so that, you know, in case a
  glitch happens you get the chance to not get killed by it).
  

- Contributed to identifying and solving issues related with certain AR-HUD functionalities. E.g.,
Audi's global drone positioning improvements (the blue arrows):

  ![Global navigation drone on Audi Q4 etronm][global-navigation-drone-audi]
    
  Positioning a global element (in this case the turn icon) in the body coordinate system (BCS)
  of the car is quite challenging and heavily dependent on the accuracy of global positioning
  sensors and their data fusion (e.g., GPS), the accuracy of navigation data (e.g., [NDS][nds]),
  local orientation sensors (e.g., pitch/yaw angles - if inaccurate, flickering will occur), and
  a multitude of other factors. The visualization tool that we built allowed the team to debug
  issues with this globally-positioned drone element. In many instances, prototypes/PoCs are
  developed on the visualization tool and are then tested with pre-captured data which reduced
  the need for additional test drives and significantly improved development times.

- Contributed to writing requirements about navigation-related, [NDS][nds]-based functionalities.

- Wrote Python scripts to parse and visualize a multitude of data dumps. Used technologies: Python, Tkinter, Plotly.

- Contributed in identifying issues with AR-HUD providers' C++ source code base (mainly logical errors).

- Worked within a highly interdisciplinary team involving software engineers, product managers, and test engineers.

# Projects

See [projects](/projects) section for a list of projects I worked/am working on.

# Education

In April 2025 I obtained a double Master's degree in computer science at the University of Passau (Germany)
and the National School of Computer Science (ENSI) (Tunisia). Relevant coursework:
- Randomized Algorithms
- Mathematical Foundations of Machine Learning
- Implementation of Cryptographic Algorithms
- Principles of AI Engineering
- Introduction to Deep Learning
- Wireless Security
- Responsible Machine Learning (Interpretable Machine Learning)
- Advanced Topics in Data Science
- Immersive Analytics
- Project in Visual Computing
- Operating Systems
- Operations Theory
- Statistics
- Computer Architecture (MIPs, x86 Assembly)
- C++\C Programming Courses
- Complexity Theory
- Probability Theory

Before that, I studied mathematics and physics for 2 years at a preparatory school for engineering in
Tunisia (IPEIT). Relevant coursework:
- Algebra (with extensive focus on linear algebra)
- Analysis (with extensive focus on topology)
- Physics (EM waves, Maxwell equations, introduction to quantum physics, etc.)
- Control Theory
- Python
- SQL databases

[resume]: https://github.com/walcht
[linkedin]: https://linkedin.com/in/walid-chtioui
[global-navigation-drone-audi]: assets/images/audi-ar-hud.jpeg
[ar-hud-static-vs-ar-images]: assets/images/ar-hud-static-vs-ar-images.jpeg
[nds]: https://nds-association.org/
[cariad]: https://cariad.technology/
[esolutions]: https://www.esolutions.de/de/
[ar-hud-audi-showcase]: https://www.youtube.com/embed/45qHqjDSZgY?si=Zblri9jZg0JwREUz
