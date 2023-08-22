# coding: utf-8
__version__ = "1.0"
__author__ = "Cristobal Pais"

# Statistics Class
# Importations
import pandas as pd
import numpy as np
import glob
import os

# Plot
import matplotlib
from matplotlib.backends.backend_pdf import PdfPages
from mpl_toolkits.axes_grid1 import make_axes_locatable
from matplotlib import rcParams
import matplotlib.pyplot as plt
#from matplotlib.pylab import *
#import matplotlib as mpl
import matplotlib.cm as cm
#import matplotlib.patches as patches
from matplotlib.colors import LinearSegmentedColormap, ListedColormap, BoundaryNorm
import seaborn as sns
#import matplotlib.colors as colors
#matplotlib.use('Qt5Agg')
from PIL.Image import open as open_image
# img = np.asarray( open_image('/home/fdo/Pictures/Lenna.png'))

# Extra
import multiprocessing
from multiprocessing import Process

# Extra
from operator import itemgetter
#import itertools
#from coord_xy import coord_xy
#from Cell2Fire.coord_xy import coord_xy        
from tqdm import tqdm
import networkx as nx
from shutil import copy2

# Cell2Fire
#import .ReadDataPrometheus as ReadDataPrometheus
from platform import system


class Statistics:
    # Initializer
    def __init__(self,
                 OutFolder="",
                 StatsFolder="",
                 MessagesPath="",
                 Rows=0,
                 Cols=0,
                 NCells=0,
                 boxPlot=True,
                 CSVs=True,
                 statsGeneral=True,
                 statsHour=True,
                 histograms=True,
                 BurntProb=True,
                 nSims=1,
                 verbose=False,
                 GGraph=None,
                 tCorrected=False,
                 pdfOutputs=False):

        self.px = 1/plt.rcParams['figure.dpi']
        self.defaultSize = (1920*self.px,1080*self.px)

        # recicle just one figure
        #self.fig = plt.figure(1)

        # Containers
        self._OutFolder = OutFolder
        self._MessagesPath = MessagesPath
        self._Rows = Rows
        self._Cols = Cols
        self._NCells = NCells
        self._boxPlot = boxPlot
        self._CSVs = CSVs
        self._statsGeneral = statsGeneral
        self._statsHour = statsHour
        self._histograms = histograms
        self._BurntProb = BurntProb
        self._nSims = nSims
        self._verbose = verbose
        self._GGraph = GGraph
        self._tCorrected = tCorrected
        self._pdfOutputs = pdfOutputs

        # Create Stats path (if needed)
        if StatsFolder != "":
            self._StatsFolder = StatsFolder
        else:
            self._StatsFolder = os.path.join(OutFolder, "Stats")
        if not os.path.exists(self._StatsFolder):
            if self._verbose:
                print("creating", self._StatsFolder)
            os.makedirs(self._StatsFolder)
        self.plt_style()


    ####################################
    #                                  #
    #            Methods               #
    #                                  #
    ####################################
    # Plot style
    def plt_style(self, b=True, l=True, r=False, t=False):
        '''
        https://matplotlib.org/stable/tutorials/introductory/customizing.html
        https://matplotlib.org/stable/gallery/subplots_axes_and_figures/figure_size_units.html
        , bbox_inches='tight', dpi=200
        '''
            #'backend':'QtAgg',
        params = { \
            'interactive': False,
            'font.size' : 16,
            'axes.labelsize' : 16,
            'axes.titlesize' : 16,
            'xtick.labelsize' : 16,
            'ytick.labelsize' : 16,
            'legend.fontsize' : 16,
            'figure.titlesize' : 18,
            'axes.spines.bottom': b,
            'axes.spines.left': l,
            'axes.spines.right': r,
            'axes.spines.top': t,
            'xtick.bottom': True,
            'ytick.left': True,
            'axes.facecolor':'white',
            'figure.facecolor':'white',
            }
        plt.rcParams.update( params)
        sns.set_theme( rc=params)
        sns.set_style( rc=params)
        sns.set_context( rc=params)
        sns.axes_style( rc=params)

    def add_alpha_channel(self, src, alpha=255):
        if src.shape[-1] == 3:
            H,W,_ = src.shape
            dst = np.empty((H,W,4), dtype=src.dtype)
            dst[:,:,:3] = np.copy(src)
            dst[:,:,3] = np.ones((H,W), dtype=src.dtype)*alpha
            return dst
        return np.copy(src)

    # Boxplot function
    def BoxPlot(self, Data, xx="Hour", yy="Burned", xlab="Hours", ylab="# Burned Cells",
                pal="Reds", title="Burned Cells Evolution", Path=None, namePlot="BoxPlot",
                swarm=True):

        # Figure
        self.plt_style()
        fig, ax = plt.subplots( num=1, figsize=self.defaultSize)

        # Title and labels
        ax.set_title(title)

        #sns.set(style="darkgrid", font_scale=1.5)
        ax = sns.boxplot(x=xx, y=yy, data=Data, linewidth=2.5, palette=pal, ax=ax).set(xlabel=xlab,ylabel=ylab)    
        if swarm:
            ax = sns.swarmplot(x=xx, y=yy, data=Data, linewidth=2.5, palette=pal, ax=ax).set(xlabel=xlab,ylabel=ylab)   

        # Save it
        if Path is None:
            Path = os.getcwd() + "/Stats/"
            if not os.path.exists(Path):
                os.makedirs(Path)

        plt.savefig(os.path.join(Path, namePlot + ".png"))
        if self._pdfOutputs:
            plt.savefig(os.path.join(Path, namePlot + ".pdf"))
        plt.clf()
        plt.close("all")


    # Final forest boxplot
    def FinalBoxPlots(self, OutFolder=None, title="Final Forest Status",
                      namePlot="FinalStats_BoxPlot"):

        # Style
        self.plt_style()
        fig, ax = plt.subplots( num=1, figsize=self.defaultSize)

        # Title and labels
        ax.set_title(title)

        # DFs containers (with results from heuristics)
        DFs = {}

        # Populate DF
        filePath = os.path.join(self._StatsFolder, "FinalStats.csv")
        DF = pd.read_csv(filePath)
        DF = DF[["Burned", "NonBurned", "Firebreak"]]

        # Plot
        my_pal = {"Burned": "r", "NonBurned": "g", "Firebreak":"b"}
        ax = sns.boxplot(data=DF, linewidth=2.5, palette=my_pal, ax=ax).set(xlabel="Final State", ylabel="Hectares")

        # Save it
        plt.savefig(os.path.join(self._StatsFolder, namePlot + ".png"))
        if self._pdfOutputs:
            plt.savefig(os.path.join(self._StatsFolder, namePlot + ".pdf"))
        plt.clf()
        plt.close("all")

    # Histograms
    def plotHistogram(self, df, NonBurned=False, xx="Hour", xmax=6, KDE=True, title="Histogram: Burned Cells",
                      Path=None, namePlot="Histogram"):

        # Modify rc parameters (Matplotlib/Seaborn)
        rcParams['patch.force_edgecolor'] = True
        rcParams['patch.facecolor'] = 'b'

        self.plt_style()
        fig, ax = plt.subplots( num=1, figsize=self.defaultSize)
        # Title and labels
        ax.set_title(title)

        # Make default histogram of sepal length
        # TODO sns.displot: This function has been deprecated and will be removed in seaborn v0.14.0. -> histplot or displot
        if KDE is True:
            g = sns.distplot(df[df[xx] == xmax]["Burned"], bins=10, kde=KDE, rug=False, ax=ax).set(xlabel="Number of Cells", ylabel="Density")
            if NonBurned is True: 
                g += sns.distplot(df[df[xx] == xmax]["NonBurned"], bins=10, kde=KDE, rug=False, ax=ax).set(xlabel="Number of Cells", ylabel="Density")
        else:
            g = sns.distplot(df[df[xx] == xmax]["Burned"], bins=10,  kde=KDE, rug=False, ax=ax).set(xlabel="Number of Cells", ylabel="Frequency")
            if NonBurned is True:
                g += sns.distplot(df[df[xx] == xmax]["NonBurned"], bins=10, kde=KDE, rug=False, ax=ax).set(xlabel="Number of Cells", ylabel="Frequency")

        # Save it
        if Path is None:
            Path = os.getcwd() + "/Stats/"
            if not os.path.exists(Path):
                os.makedirs(Path)

        plt.savefig(os.path.join(Path, namePlot + ".png"))
        plt.clf()
        plt.close('all')

    # Burnt Probability Heatmap
    #   cbarF seems to indicate no legend when true.
    def BPHeatmap(self, WeightedScar, Path=None, nscen=10, sq=False, namePlot="BP_HeatMap",
                  Title=None, cbarF=True, ticks=100, transparent=False):
        self.plt_style()
        fig, ax = plt.subplots( num=1, figsize=self.defaultSize)

        # Title and labels
        if Title is None:
            ax.set_title("Fire Probability Heatmap (nscen="+str(nscen)+")")
        else:
            ax.set_title(Title)

        # Modify existing map to have white values
        cmap = cm.get_cmap('RdBu_r')
        lower = plt.cm.seismic(np.ones(1)*0.50)  # Original is ones
        upper = cmap(np.linspace(0.5, 1, 100))
        colors = np.vstack((lower,upper))
        tmap = LinearSegmentedColormap.from_list('terrain_map_white', colors)

        # Create Heatmap
        ax = sns.heatmap(WeightedScar, xticklabels=ticks, yticklabels=ticks, linewidths=0.0, linecolor="w",
                         square=sq, cmap=tmap, vmin=0.0, vmax=1, annot=False, cbar=False, ax=ax)#cbarF)

        # If cbarF is false, we want the legend
        if cbarF == False:
            sm = plt.cm.ScalarMappable(cmap=tmap)#, norm=plt.Normalize(vmin=np.min(0), vmax=np.max(1)))
            sm._A = []
            divider = make_axes_locatable(ax)
            cax1 = divider.append_axes("right", size="5%", pad=0.15)
            plt.colorbar(sm, cax=cax1)

        # Save it
        if Path is None:
            Path = os.getcwd() + "/Stats/"
            if not os.path.exists(Path):
                os.makedirs(Path)

        for _, spine in ax.spines.items():
            spine.set_visible(True)

        plt.savefig(os.path.join(Path, namePlot + ".png"), pad_inches=0, transparent=transparent)
        if self._pdfOutputs:
            plt.savefig(os.path.join(Path, namePlot + ".pdf"), format='pdf', pad_inches=0, transparent=transparent)
        plt.clf()
        plt.close('all')
    
    # ROS Heatmap
    def ROSHeatmap(self, ROSM, Path=None, nscen=1, sq=True, namePlot="ROS_HeatMap",
                   Title=None, cbarF=True, ticks="auto", transparent=False,
                   annot=False, lw=0.01, vmin=0, vmax=None):
        self.plt_style()
        fig, ax = plt.subplots( num=1, figsize=self.defaultSize)

        # Title and labels
        if Title is None:
            ax.set_title("ROS Heatmap (nscen="+str(nscen)+")")
        else:
            ax.set_title(Title)

        # Modify existing map to have white values
        cmap = cm.get_cmap('RdBu_r')
        lower = plt.cm.seismic(np.ones(1)*0.50)  # Original is ones 
        upper = cmap(np.linspace(0.5, 1, 100))
        colors = np.vstack((lower,upper))
        tmap = LinearSegmentedColormap.from_list('terrain_map_white', colors)
        '''
        #lower = plt.cm.seismic(np.ones(100)*0.50)  # Original is ones
        upper = cmap(np.linspace(1 - 0.5, 1, 100))
        colors = np.vstack((upper,))
        tmap = matplotlib.colors.LinearSegmentedColormap.from_list('terrain_map_white', colors)
        '''

        # Limits
        if vmax is None:
            vmax = np.max(ROSM)

        ax = sns.heatmap(ROSM, xticklabels=ticks, yticklabels=ticks, linewidths=lw, linecolor="w",
                         square=sq, cmap=tmap, vmin=vmin, vmax=vmax, annot=False, cbar=False, ax=ax)
        sm = plt.cm.ScalarMappable(cmap=tmap)
        sm._A = []
        divider = make_axes_locatable(ax)
        cax1 = divider.append_axes("right", size="5%", pad=0.15)
        plt.colorbar(sm, cax=cax1)  

        # Save it
        if Path is None:
            Path = os.getcwd() + "/Stats/"
            if not os.path.exists(Path):
                os.makedirs(Path)

        for _, spine in ax.spines.items():
            spine.set_visible(True)

        plt.savefig(os.path.join(Path, namePlot + ".png"), pad_inches=0, transparent=transparent)
        if self._pdfOutputs:
            plt.savefig(os.path.join(Path, namePlot + ".pdf"), format='pdf', pad_inches=0, transparent=transparent)
        plt.clf() 
        plt.close('all')
    
    # ROS Matrix individual
    def ROSMatrix_ind(self, nSim):
        msgFileName = "MessagesFile0" if (nSim < 10) else "MessagesFile"
        DF = pd.read_csv(os.path.join(self._MessagesPath, msgFileName), delimiter=",", header=None,)
        DF.columns = ["i", "j", "time", "ROS"]

        # Array
        ROSM = np.zeros(self._Rows * self._Cols)

        # Fill
        for j in DF["j"]:
            ROSM[j-1] = DF[DF["j"] == j]["ROS"].values[0]
        ROSM = ROSM.reshape((self._Rows, self._Cols))

        # Create plots folder
        PlotPath = os.path.join(self._OutFolder, "Plots", "Plots" + str(nSim))
        if os.path.isdir(PlotPath) is False:
            os.makedirs(PlotPath)

        # Heatmap
        self.ROSHeatmap(ROSM, Path=PlotPath, nscen=1, sq=True, namePlot="ROS_Heatmap",
                        Title="ROS Heatmap", cbarF=True)


    # ROS Matrix
    def ROSMatrix_AVG(self, nSims): 
        # Container 
        ROSMs = {}

        # Read all files
        for nSim in range(1, nSims + 1):
            msgFileName = "MessagesFile0" if (nSim < 10) else "MessagesFile"
            msgFileName = msgFileName + str(nSim) + '.csv'
            DF = pd.read_csv(os.path.join(self._MessagesPath, msgFileName), delimiter=",", header=None,)
            DF.columns = ["i", "j", "time", "ROS"]

            # Array
            ROSM = np.zeros(self._Rows * self._Cols)

            # Fill
            for j in DF["j"]:
                ROSM[j-1] = DF[DF["j"] == j]["ROS"].values[0]
            ROSM = ROSM.reshape((self._Rows, self._Cols))

            # Save
            ROSMs[nSim] = ROSM
        
        # AVG ROS
        AVGROSM = np.zeros((Rows, Cols))
        for k in ROSMs.keys():
            if k == 1:
                AVGROSM = ROSMs[k].copy()
            else:
                AVGROSM += ROSMs[k]
        AVGROSM = AVGROSM / k

        # Create plots folder
        PlotPath = os.path.join(self._OutFolder, "Plots", "Plots" + str(nSim))
        if os.path.isdir(PlotPath) is False:
            os.makedirs(PlotPath)

        # Heatmap
        self.ROSHeatmap(AVGROSM, 
                        Path=PlotPath,
                        nscen=1,
                        sq=True,
                        namePlot="AVG_ROS_Heatmap", 
                        Title="AVG ROS Heatmap",
                        cbarF=True)

    # Generate G graph
    def GGraphGen(self, full=False):
        # Graph generation
        nodes = range(1, self._NCells + 1)

        # We build a Digraph (directed graph, general graph for the instance)
        self._GGraph = nx.DiGraph()

        # We add nodes to the list
        self._GGraph.add_nodes_from(nodes)
        for n in nodes:
            self._GGraph.nodes[n]["ros"] = 0
            self._GGraph.nodes[n]["time"] = 0
            self._GGraph.nodes[n]["count"] = 0

        # If full, get edges
        if full:
            for k in range(1, self._nSims + 1):
                msgFileName = "MessagesFile0" if (k < 10) else "MessagesFile"
                HGraphs = nx.read_edgelist(path=self._MessagesPath + '/' + msgFileName + str(k) + '.csv',
                                           create_using=nx.DiGraph(),
                                           nodetype=int,
                                           data=[('time', float), ('ros', float)],
                                           #data=[('time', int), ('ros', float)],
                                           delimiter=',')

                for e in HGraphs.edges():
                    if self._GGraph.has_edge(*e):
                        self._GGraph.get_edge_data(e[0], e[1])["weight"] += 1
                        self._GGraph.nodes[e[1]]["ros"] += HGraphs[e[0]][e[1]]["ros"]
                        self._GGraph.nodes[e[1]]["time"] += HGraphs[e[0]][e[1]]["time"]
                        self._GGraph.nodes[e[1]]["count"] += 1
                    else:
                        self._GGraph.add_weighted_edges_from([(*e,1.)])

        # Average ROS, time
        for n in nodes:
            if self._GGraph.nodes[n]['count'] > 0:
                self._GGraph.nodes[n]['ros'] /= self._GGraph.nodes[n]['count']
                self._GGraph.nodes[n]['time'] /= self._GGraph.nodes[n]['count']


    # Fire Spread evolution plot (global sims)
    def GlobalFireSpreadEvo(self, CoordCells, onlyGraph=True, version=0):
        # V0 Frequency
        if self._GGraph is None:
            self.GGraphGen(full=True)

        coord_pos = dict() # Cartesian coordinates
        for i in self._GGraph.nodes:
            coord_pos[i] = CoordCells[i-1] + 0.5

        plt_style(r=True,t=True)
        fig, ax = plt.subplots( num=1, figsize=self.defaultSize)
        for _, spine in ax.spines.items():
            spine.set_visible(True)


        # Dimensionamos el eje X e Y
        #plt.axis([-1, self._Rows, -1, self._Cols])

        # Print nodes
        if onlyGraph is False:
            nx.draw_networkx_nodes(self._GGraph, pos = coord_pos,
                                   node_size = 5,
                                   nodelist = list(self._GGraph.nodes),
                                   node_shape='s',
                                   node_color = Colors, ax=ax)

        edges = self._GGraph.edges()
        weights = [self._GGraph[u][v]['weight'] for u,v in edges]
        outname = ""

        # Check non-zeros
        all_zeros = not np.asarray(weights).any()

        if all_zeros is False:
            if version == 0:
                # Fixed edge color (red), different scaled width
                outname = "NWFreq"
                plt.title(r"Global Propagation Tree ${GT}$ ($|R| =$ " +str(self._nSims)+")", y=1.08)

                nx.draw_networkx_edges(self._GGraph, pos = coord_pos, edge_color = 'r', node_size = 0,
                                       width = weights/np.max(weights), arrowsize=3, ax=ax)
                #nx.draw(self._GGraph, with_labels=False, pos = coord_pos, node_color='w', node_size=0, 
                #    edge_color='r', width=weights/np.max(weights), arrows=True, arrowsize=3, ax=ax)

            if version == 4:
                # Edge = Frequency, width = frequency
                outname = "CFreq_NWFreq"
                plt.title(r"Global Propagation Tree ${GT}$ ($|R| =$ " +str(self._nSims)+")", y=1.08)
                nx.draw_networkx_edges(self._GGraph, pos = coord_pos, edge_color = weights, edge_cmap=plt.cm.Reds,
                                       width = weights/np.max(weights), arrowsize=3, node_size = 0, ax=ax)
                sm = plt.cm.ScalarMappable(cmap=plt.cm.Reds, norm=plt.Normalize(vmin=np.min(weights), vmax=np.max(weights)))
                sm._A = []
                #plt.colorbar(sm)
                #nx.draw(self._GGraph, with_labels=False, pos = coord_pos, node_color='w', node_size=0, edge_cmap=plt.cm.Reds,
                #        edge_color=weights, width=weights/np.max(weights), arrows=True, arrowsize=3, ax=ax)


            if version == 1:
                # Edge = Frequency, width = frequency
                outname = "CNFreq_NWFreq"
                plt.title(r"Global Propagation Tree ${GT}$ ($|R| =$ " +str(self._nSims)+") - Freq Color|Width", y=1.08)
                nx.draw_networkx_edges(self._GGraph, pos = coord_pos, edge_color = weights/np.max(weights), edge_cmap=plt.cm.Reds,
                                      width = weights/np.max(weights), arrowsize=3, node_size = 0, ax=ax)
                sm = plt.cm.ScalarMappable(cmap=plt.cm.Reds, norm=plt.Normalize(vmin=np.min(0), vmax=np.max(1)))
                sm._A = []
                #plt.colorbar(sm)
                #nx.draw(self._GGraph, with_labels=False, pos = coord_pos, node_color='w', node_size=0, edge_cmap=plt.cm.Reds,
                #        edge_color=weights/np.max(weights), width=weights/np.max(weights), arrows=True, arrowsize=3, ax=ax)


            if version == 2:
                # Edge = frequency, fixed width
                outname = "CFreq"
                plt.title(r"Global Propagation Tree ${GT}$ ($|R| =$ " +str(self._nSims)+") - Freq Color", y=1.08)
                nx.draw_networkx_edges(self._GGraph, pos = coord_pos, edge_color = weights, edge_cmap=plt.cm.Reds,
                                      width = 1.0, arrowsize=3, node_size = 0, ax=ax)
                sm = plt.cm.ScalarMappable(cmap=plt.cm.Reds, norm=plt.Normalize(vmin=np.min(weights), vmax=np.max(weights)))
                sm._A = []
                #nx.draw(self._GGraph, with_labels=False, pos = coord_pos, node_color='w', node_size=0, edge_cmap=plt.cm.Reds,
                #        edge_color=weights, width=1.0, arrows=True, arrowsize=3, ax=ax)

            #Title
            plt.axis('scaled')
            if version != 0:
                divider = make_axes_locatable(ax)
                cax1 = divider.append_axes("right", size="5%", pad=0.15)
                plt.colorbar(sm, cax=cax1)
                
            plt.savefig(os.path.join(self._StatsFolder, "SpreadTree_FreqGraph_" + outname + ".png"), transparent=False)
            if self._pdfOutputs:
                plt.savefig(os.path.join(self._StatsFolder, "SpreadTree_FreqGraph_" + outname + ".pdf"), format='pdf', transparent=False)
            plt.clf()
            plt.close('all')

    # Fire Spread evolution plots (per sim)
    def SimFireSpreadEvo(self, nSim, CoordCells, Colors, H=None, version=0,
                         print_graph=True, analysis_degree=True, onlyGraph=False):
        # V1
        # If no graph, read it
        if H is None:
            msgFileName = "MessagesFile0" if (nSim < 10) else "MessagesFile"
            H = nx.read_edgelist(path = os.path.join(self._MessagesPath, msgFileName + str(nSim) + ".csv"),
                                 create_using = nx.DiGraph(),
                                 nodetype = int,
                                 data = [('time', float), ('ros', float)],
                                 delimiter=',')

        # Cartesian positions
        coord_pos = dict()

        # Generate G graph (empty)
        if self._GGraph is None:
            self.GGraphGen()
        for i in self._GGraph.nodes:
            coord_pos[i] = CoordCells[i-1] + 0.5

        # We generate the plot
        if print_graph:

            plt_style(r=True,t=True)
            fig, ax = plt.subplots( num=1, figsize=self.defaultSize)

            # Dimensionamos el eje X e Y
            plt.axis([-1, self._Rows, -1, self._Cols])

            # Print graph and nodes
            if onlyGraph is False:
                nx.draw_networkx_nodes(self._GGraph, pos = coord_pos,
                                       node_size = 5,
                                       nodelist = list(self._GGraph.nodes),
                                       node_shape='s',
                                       node_color = Colors, ax=ax)

            #nx.draw(H, with_labels=False, pos = coord_pos, node_color='w', node_size=0,
            #        edge_color= 'r', width=0.5, edge_cmap=plt.cm.Reds,
            #        arrows=True, arrowsize=3, ax=ax)

            nx.draw_networkx_edges(H, pos = coord_pos, edge_color = 'r', width = 0.5, arrowsize=3, node_size=0, ax=ax)
            
            #Title
            plt.title("Propagation Tree")
            plt.axis('scaled')

            # Save plot
            PlotPath = os.path.join(self._OutFolder, "Plots", "Plots" + str(nSim))
            if os.path.isdir(PlotPath) is False:
                os.makedirs(PlotPath)
            
            plt.savefig(os.path.join(PlotPath, "PropagationTree" + str(nSim) +".png"), edgecolor='b', transparent=False)
            if self._pdfOutputs:
                plt.savefig(os.path.join(PlotPath, "PropagationTree" + str(nSim) +".pdf"), format='pdf', edgecolor='b', transparent=False)
            plt.clf()
            plt.close('all')
            
        # Hitting times and ROSs
        if analysis_degree is True:
            dg_ros_out = sorted(list(H.out_degree(weight = 'ros')), key = itemgetter(1), reverse = True)
            dg_time_out = sorted(list(H.out_degree(weight = 'time')), key = itemgetter(1))

            PlotPath = os.path.join(self._OutFolder, "Plots", "Plots" + str(nSim))
            if os.path.isdir(PlotPath) is False:
                os.makedirs(PlotPath)

            plt.figure(2)
            dg_ros = dict(H.degree(weight='ros'))
            plt.hist(dg_ros.values())
            plt.title('ROS hit Histogram')
            plt.savefig(os.path.join(PlotPath, 'HitROS_Histogram.png'))
            plt.clf()
            
            plt.figure(3)
            dg_time = dict(H.degree(weight='time'))
            plt.hist(dg_time.values())
            plt.title('Time hit Histogram')
            plt.savefig(os.path.join(PlotPath, 'HitTime_Histogram.png'))
            plt.clf()

        plt.close("all")


    # Fire Spread evolution plots (per sim, version 2)
    def SimFireSpreadEvoV2(self, nSim, CoordCells, Colors, H=None, version=0, onlyGraph=True):
        if H is None:
            msgFileName = "MessagesFile0" if (nSim < 10) else "MessagesFile"
            H = nx.read_edgelist(path = os.path.join(self._MessagesPath, msgFileName + str(nSim) + ".csv"),
                                 create_using = nx.DiGraph(),
                                 nodetype = int,
                                 data = [('time', float), ('ros', float)],
                                 delimiter=',')

        # If MessageFile is empty, skip
        if len(H.nodes) > 0:
            # Cartesian positions
            coord_pos = dict()

            # Generate G graph (empty)
            if self._GGraph is None:
                self.GGraphGen()
            for i in self._GGraph.nodes:
                coord_pos[i] = CoordCells[i-1] + 0.5

            plt_style(r=True,t=True)
            fig, ax = plt.subplots( num=1, figsize=self.defaultSize)

            # No nodes
            if onlyGraph is False:
                nx.draw_networkx_nodes(self._GGraph, pos = coord_pos,
                                       node_size = 4,
                                       nodelist = list(G.nodes),
                                       node_shape='s',
                                       node_color = Colors, ax=ax)

            # Dimensionamos el eje X e Y
            plt.axis([-1, self._Rows, -1, self._Cols])

            # Simple red
            if version == 0:
                nx.draw_networkx_edges(H, pos = coord_pos, node_size=0,
                        edge_color= 'r', width=0.5, edge_cmap=plt.cm.Reds,
                        arrows=True, arrowsize=3, ax=ax, label="ROS messages")


            # Different plot versions
            elif version != 0 and H is not None:
                edges = H.edges()
                ross = [H[u][v]['ros'] for u,v in edges]
                times = [H[u][v]['time'] for u,v in edges]

                # Edge color = ROSs
                if version == 1:
                    plt.title("Propagation Tree: hitting ROS [m/min]")
                    #nx.draw_networkx_edges(H, label=None, pos = coord_pos, node_size=0,
                    nx.draw_networkx_edges(H, pos = coord_pos,
                        edge_color=ross, width=1.0, edge_cmap=plt.cm.Reds,
                        arrows=True, arrowsize=3, ax=ax)
                    sm = plt.cm.ScalarMappable(cmap=plt.cm.Reds, norm=plt.Normalize(vmin=np.min(ross), vmax=np.max(ross)))

                # Edge color = hit Times (normalized)
                if version == 2:
                    plt.title("Propagation Tree: traveling times [min]")
                    #nx.draw_networkx_edges(H, label=None, pos = coord_pos, node_size=0,
                    nx.draw_networkx_edges(H, pos = coord_pos,
                            edge_color=times, width=1.0, edge_cmap=plt.cm.Reds,  #edge_color=times/np.max(times)
                            arrows=True, arrowsize=3, ax=ax)
                    sm = plt.cm.ScalarMappable(cmap=plt.cm.Reds, norm=plt.Normalize(vmin=np.min(times), vmax=np.max(times)))

                # Edge color = Weights (Ross) and width = hit times (normalized)
                if version == 3:
                    plt.title("Propagation Tree: ROS (c) and times (w)")
                    #nx.draw_networkx_edges(H, label=None, pos = coord_pos, node_size=0,
                    nx.draw_networkx_edges(H, pos = coord_pos,
                            edge_color= ross/np.max(ross), width= times/np.max(times), edge_cmap=plt.cm.Reds,
                            arrows=True, arrowsize=3, ax=ax)
                    sm = plt.cm.ScalarMappable(cmap=plt.cm.Reds, norm=plt.Normalize(vmin=np.min(0), vmax=np.max(1)))


            # Add legend and coordinates (if needed)
            # plt.legend()
            XCoord = None
            YCoord = None

            # Create plots folder
            PlotPath = os.path.join(self._OutFolder, "Plots", "Plots" + str(nSim))
            if os.path.isdir(PlotPath) is False:
                os.makedirs(PlotPath)

            # Save Figure
            plt.axis('scaled')
            sm._A = []
            #plt.colorbar(sm, fraction=0.046, pad=0.04)
            #cax = ax.add_axes([ax.get_position().x1+0.01,ax.get_position().y0,0.02,ax.get_position().height])

            #ax.set_aspect('auto')
            divider = make_axes_locatable(ax)
            cax1 = divider.append_axes("right", size="5%", pad=0.05)
            plt.colorbar(sm, cax=cax1)

            plt.savefig(os.path.join(PlotPath, "FireSpreadTree" + str(nSim) + "_" + str(version) + ".png"), transparent=False)
            if self._pdfOutputs:
                plt.savefig(os.path.join(PlotPath, "FireSpreadTree" + str(nSim) + "_" + str(version) + ".pdf"), format='pdf', transparent=False)
            plt.clf()
            plt.close('all')
        
    
    # Individual BP maps (for plotting the evolution of the fire)
    def plotEvo(self):
        # If nSims = -1, read the output folder
        if self._nSims == -1:
            # Read folders with Grids (array with name of files)
            Grids = glob.glob(self._OutFolder + 'Grids/')
            self._nSims = len(Grids)

        # Grids files (final scars)
        a = 0

        #print("NSims:", self._nSims)

        # Stats per simulation
        for i in tqdm(range(self._nSims)):
            GridPath = os.path.join(self._OutFolder, "Grids", "Grids"+str(i + 1))
            GridFiles = os.listdir(GridPath)
            #print("GridFiles:", GridFiles, "\nSim:", i+1)

            # Reset container
            a = 0

            for j in range(len(GridFiles)):
                # Reset container
                a = 0

                #print("J from GridGiles:", j, GridFiles[j])
                if len(GridFiles) > 0:
                    a = pd.read_csv(GridPath +"/"+ GridFiles[j], delimiter=',', header=None).values

                else:
                    if i != 0:
                        a = np.zeros([a.shape[0], a.shape[1]]).astype(np.int64)

                    else:
                        a = np.zeros([self._Rows,self._Cols]).astype(np.int64)

                # Set Firebreak to null prob
                a[a == 2] = 0
                #print("a:", a)

                # Generate BPHeatmap
                PlotPath = os.path.join(self._OutFolder, "Plots/Plots" + str(i + 1))
                if os.path.isdir(PlotPath) is False:
                    os.makedirs(PlotPath)

                num = str(j+1).zfill(2)
                self.BPHeatmap(a, Path=PlotPath, nscen=1, sq=True, namePlot="Fire" + num,
                               Title="Burned Cells Fire Period " + str(j + 1), cbarF=True, ticks=False,
                               transparent=True)
                               #Title="Fire Period " + str(j + 1), cbarF=False, ticks=False,

    # Plot full forest (all cells and colors)
    def ForestPlot(self, LookupTable, data, Path, namePlot="InitialForest"):
        # Colors dictionary (container)
        myColorsD = {}

        # DF from lookup table
        DFColors = pd.read_csv(LookupTable)
        ColorsID = DFColors["grid_value"].values

        # Populate dict
        MColors = DFColors[[" r", " g", " b"]].values / 255.0
        MColors = [tuple(np.concatenate((i,[1.0]))) for i in MColors]

        aux = 0
        myColorsD = {}
        for i in ColorsID:
            myColorsD[i] = MColors[aux]
            aux += 1
        myColorsD[9999] = (1.0, 1.0, 1.0, 1.0)
        #myColorsD[-9999] = (1.0, 1.0, 1.0, 1.0)
        #myColorsD[0] = (1.0, 1.0, 1.0, 1.0)
        myColorsD[-1] = (6/255., 150/255., 165/255., 1.0)

        mykeys = np.unique(data)

        aux = 0
        for val in mykeys:
            data[data == val] = aux
            aux += 1

        #print("Unique values:", mykeys)
        myColors = [myColorsD[x] for x in mykeys]
        #print("myColors:", myColors)

        plt_style(r=False,t=False)
        fig, ax = plt.subplots( num=1, figsize=self.defaultSize)

        # Title and labels
        ax.set_title(" ")

        if len(mykeys) > 1:
            cmap = LinearSegmentedColormap.from_list('Custom', myColors, len(myColors))
            ax = sns.heatmap(data, cmap=cmap, linewidths=.0, linecolor='lightgray', annot=False, cbar=False,
                             square=True, xticklabels=False, yticklabels=False, ax=ax)

        else:
            cmap = ListedColormap(myColors)
            boundaries = [-1, 1]
            norm = BoundaryNorm(boundaries, cmap.N, clip=True)
            ax = sns.heatmap(data, cmap=cmap, linewidths=.0, linecolor='lightgray',
                             annot=False, cbar=False, norm=norm, square=True,   # Testing new options for combining
                             xticklabels=False, yticklabels=False, ax=ax)


        # Only y-axis labels need their rotation set, x-axis labels already have a rotation of 0
        _, labels = plt.yticks()
        plt.setp(labels, rotation=0)
        
        plt.savefig(os.path.join(Path, namePlot + ".png"), pad_inches=0, transparent=False)
        if self._pdfOutputs:
            plt.savefig(os.path.join(Path, namePlot + ".pdf"), format='pdf', pad_inches=0, transparent=False)
        plt.clf()
        plt.close('all')

    # Merge evolution plots with background (Initial forest)
    def combinePlot(self, p1, BackgroundPath, fileN, Sim):
        ''' moved outside
        # Read Forest
        ForestFile = os.path.join(BackgroundPath, "InitialForest.png")
        p1 = cv2.imread(ForestFile) 
        '''

        # Read Evo plot
        fstr = str(fileN).zfill(2)
        PathFile = os.path.join(BackgroundPath, "Plots", "Plots"+ str(Sim), "Fire" + fstr + ".png")
        p2 = np.asarray( open_image( PathFile))

        # Alpha channels
        p2 = self.add_alpha_channel(p2)
        p2[np.all(p2 >= [230, 230, 230, 230], axis=2)] = [0, 0, 0, 1]

        self.plt_style(r=False,t=False)
        fig, ax = plt.subplots( num=1, figsize=self.defaultSize)

        # Axis
        # TODO replace gca with ax.
        plt.gca().set_axis_off()
        plt.subplots_adjust(top = 1, bottom = 0,
                        right = 1, left = 0,
                        hspace = 0, wspace = 0)
        plt.margins(0,0)
        plt.gca().xaxis.set_major_locator(plt.NullLocator())
        plt.gca().yaxis.set_major_locator(plt.NullLocator())

        # Create plot
        plt.imshow(p1, zorder=0)
        plt.imshow(p2, zorder=1)
        plt.savefig(PathFile, pad_inches=0, transparent=False)
        if self._pdfOutputs:
            PathFile = os.path.join(BackgroundPath, "Plots", "Plots"+ str(Sim), "Fire" + fstr + ".pdf")
            plt.savefig(PathFile, pad_inches=0, transparent=False, format='pdf')
        plt.clf()
        plt.close('all')



    # Merge the plots
    def mergePlot(self, multip=True):
        # Read Forest
        ForestFile = os.path.join( self._OutFolder, "InitialForest.png")
        p1 = np.asarray( open_image( ForestFile))
        p1 = add_alpha_channel(p1)
        # Stats per simulation
        for i in tqdm(range(self._nSims)):
            PlotPath = os.path.join(self._OutFolder, "Plots", "Plots" + str(i + 1))
            PlotFiles = glob.glob(os.path.join(PlotPath, 'Fire[0-9]*.*'))

            if multip is False:
                for (j, _) in enumerate(PlotFiles):
                    self.combinePlot( p1, self._OutFolder, j + 1, i + 1)  
            
            else:
                if system()!='Windows':
                    # Multiprocess
                    jobs = []

                    for (j, _) in enumerate(PlotFiles):
                        p = Process(target=self.combinePlot, args=( p1, self._OutFolder, j + 1, i + 1,))
                        jobs.append(p)
                        p.start()

                    # complete the processes
                    for job in jobs:
                        job.join()            
                else:
                    # MonopSerial
                    for (j, _) in enumerate(PlotFiles):
                        self.combinePlot( p1, self._OutFolder, j + 1, i + 1)  
    
    # General Stats (end of the fire stats per scenario) 
    def GeneralStats(self):
        # If nSims = -1, read the output folder
        if self._nSims == -1:
            # Read folders with Grids (array with name of files)
            Grids = glob.glob(self._OutFolder + 'Grids/')
            self._nSims = len(Grids)

        # Grids files (final scars)
        a = 0
        b = []
        statDict = {}
        statDF = pd.DataFrame(columns=[["ID", "NonBurned", "Burned", "Firebreak"]])

        # Stats per simulation
        for i in range(self._nSims):
            GridPath = os.path.join(self._OutFolder, "Grids", "Grids"+str(i + 1))
            GridFiles = os.listdir(GridPath)
            #print(GridPath, GridFiles)
            if len(GridFiles) > 0:
                a = pd.read_csv(GridPath +"/"+ GridFiles[-1], delimiter=',', header=None).values
                b.append(a)
                statDict[i] = {"ID": i+1,
                               "NonBurned": len(a[(a == 0) | (a == 2)]),
                               "Burned": len(a[a == 1]),
                               "Firebreak": len(a[a == -1])}
            else:
                if i != 0:
                    a = np.zeros([a.shape[0], a.shape[1]]).astype(np.int64)
                    b.append(a)
                    statDict[i] = {"ID": i+1,
                                   "NonBurned": len(a[(a == 0) | (a == 2)]),
                                   "Burned": len(a[a == 1]),
                                   "Firebreak": len(a[a == -1])}
                else:
                    a = np.zeros([self._Rows,self._Cols]).astype(np.int64)
                    b.append(a)
                    statDict[i] = {"ID": i+1,
                                   "NonBurned": len(a[(a == 0) | (a == 2)]),
                                   "Burned": len(a[a == 1]),
                                   "Firebreak": len(a[a == -1])}

        # Generate CSV files
        if self._CSVs:
            # Dict to DataFrame
            A = pd.DataFrame(data=statDict, dtype=np.int32)
            A = A.T
            A = A[["ID", "NonBurned", "Burned", "Firebreak"]]
            Aux = (A["NonBurned"] + A["Burned"] + A["Firebreak"])
            A["%NonBurned"], A["%Burned"], A["%Firebreak"] = A["NonBurned"] / Aux, A["Burned"] / Aux, A["Firebreak"] / Aux
            A.to_csv(os.path.join(self._StatsFolder, "FinalStats.csv"),
                     columns=A.columns, index=False, header=True,
                     float_format='%.3f')

            # Final forest status boxplot
            self.FinalBoxPlots(OutFolder=self._StatsFolder)

            # Info
            if self._verbose:
                print("General Stats:\n", A)
                print(A[["Burned","Firebreak"]].std())

            # General Summary
            SummaryDF = A.describe()
            SummaryDF.to_csv(os.path.join(self._StatsFolder, "General_Summary.csv"), header=True,
                             index=True, columns=SummaryDF.columns, float_format='%.3f')
            if self._verbose:
                print("Summary DF:\n", SummaryDF)

        # Burnt Probability Heatmap
        if self._BurntProb:
            # Compute the global weighted Scar for heatmap
            #print("self._StatsFolder:", self._StatsFolder)
            WeightedScar = 0
            for i in range(len(b)):
                WeightedScar += b[i]
            WeightedScar =  WeightedScar / len(b)

            # Set Firebreak to null prob
            WeightedScar[WeightedScar == 2] = 0

            if self._verbose:
                print("Weighted Scar:\n", WeightedScar)

            # Generate BPHeatmap
            ticks = 5
            #print("self._Rows:", self._Rows)
            if self._Rows < 10:
                ticks = 2

            elif self._Rows >= 10  and self._Rows <= 100:
                ticks = 10

            elif self._Rows > 100 and self._Rows <= 1000:
                ticks = 100

            else:
                ticks = 250
            #print("Ticks", ticks)
            #self.BPHeatmap(WeightedScar, Path=self._StatsFolder, nscen=self._nSims, sq=True, ticks=ticks)
            self.BPHeatmap(WeightedScar, Path=self._StatsFolder, nscen=self._nSims, sq=True, ticks=ticks, cbarF=False)

            # Save BP Matrix
            np.savetxt(os.path.join(self._StatsFolder, "BProb.csv"), WeightedScar,
                       delimiter=" ", fmt="%.3f")

    # Hourly stats (comparison of each hour evolution per fire)
    def HourlyStats(self):
        # If nSims = -1, read the output folder
        if self._nSims == -1:
            # Read folders with Grids (array with name of files)
            Grids = glob.glob(self._OutFolder + 'Grids/')
            self._nSims = len(Grids)

        # Correct the number of grids (equal to all replications)
        if self._tCorrected:
            maxStep = 0
            for i in range(self._nSims):
                GridPath = os.path.join(self._OutFolder, "Grids", "Grids"+str(i+1))
                GridFiles = os.listdir(GridPath)
                if len(GridFiles) > maxStep:
                    maxStep = len(GridFiles)

            for i in range(self._nSims):
                GridPath = os.path.join(self._OutFolder, "Grids", "Grids"+str(i+1))
                GridFiles = os.listdir(GridPath)
                if len(GridFiles) < maxStep:
                    for j in range(len(GridFiles), maxStep):
                        file = 'ForestGrid{:02d}.csv'.format(j)
                        copy2(os.path.join(GridPath, GridFiles[-1]),
                              os.path.join(GridPath, file))


        # Grids files (per hour)
        ah = 0
        bh = {}
        statDicth = {}
        statDFh = pd.DataFrame(columns=[["ID", "NonBurned", "Burned", "Firebreak"]])
        for i in range(self._nSims):
            GridPath = os.path.join(self._OutFolder, "Grids", "Grids"+str(i+1))
            GridFiles = os.listdir(GridPath)
            if len(GridFiles) > 0:
                for j in range(len(GridFiles)):
                    ah = pd.read_csv(GridPath +"/"+ GridFiles[j], delimiter=',', header=None).values
                    bh[(i,j)] = ah
                    statDicth[(i,j)] = {"ID": i+1,
                                       "NonBurned": len(ah[(ah == 0) | (ah == 2)]),
                                       "Burned": len(ah[ah == 1]),
                                       "Firebreak": len(ah[ah == -1]),
                                       "Hour": j+1}
            else:
                if i != 0:
                    ah = np.zeros([ah.shape[0], ah.shape[1]]).astype(np.int64)
                    bh[(i,j)] = ah
                    statDicth[(i,j)] = {"ID": i+1,
                                        "NonBurned": len(ah[(ah == 0) | (ah == 2)]),
                                        "Burned": len(ah[ah == 1]),
                                        "Firebreak": len(ah[ah == -1]),
                                        "Hour": j+1}
                else:
                    ah = np.zeros([self._Rows,self.Cols]).astype(np.int64)
                    bh[(i,0)] = ah
                    statDicth[(i,0)] = {"ID": i+1,
                                        "NonBurned": len(ah[(ah == 0) | (ah == 2)]),
                                        "Burned": len(ah[ah == 1]),
                                        "Firebreak": len(ah[ah == -1]),
                                        "Hour": 0 + 1}

        # Generate CSV files
        if self._CSVs:
            # Dict to DataFrame
            Ah = pd.DataFrame(data=statDicth, dtype=np.int32)
            Ah = Ah.T
            Ah[["Hour", "NonBurned", "Burned", "Firebreak"]] = Ah[["Hour", "NonBurned", "Burned", "Firebreak"]].astype(np.int32)
            Ah = Ah[["ID", "Hour", "NonBurned", "Burned", "Firebreak"]]
            Aux = (Ah["NonBurned"] + Ah["Burned"] + Ah["Firebreak"])
            Ah["%NonBurned"], Ah["%Burned"], Ah["%Firebreak"] = Ah["NonBurned"] / Aux, Ah["Burned"] / Aux, Ah["Firebreak"] / Aux
            Ah.to_csv(os.path.join(self._StatsFolder, "HourlyStats.csv"), columns=Ah.columns, index=False,
                      header=True, float_format='%.3f')
            if self._verbose:
                print("Hourly Stats:\n",Ah)

            # Hourly Summary
            SummaryDF = Ah[["NonBurned", "Burned", "Firebreak", "Hour"]].groupby('Hour')["NonBurned", "Burned", "Firebreak"].mean()
            SummaryDF.rename(columns={"NonBurned":"AVGNonBurned", "Burned":"AVGBurned", "Firebreak":"AVGFirebreak"}, inplace=True)
            SummaryDF[["STDBurned", "STDFirebreak"]] = Ah[["NonBurned", "Burned", "Firebreak", "Hour"]].groupby('Hour')["Burned","Firebreak"].std()[["Burned","Firebreak"]]
            SummaryDF[["MaxNonBurned", "MaxBurned"]] = Ah[["NonBurned", "Burned", "Firebreak", "Hour"]].groupby('Hour')["NonBurned", "Burned"].max()[["NonBurned", "Burned"]]
            SummaryDF[["MinNonBurned", "MinBurned"]] = Ah[["NonBurned", "Burned", "Firebreak", "Hour"]].groupby('Hour')["NonBurned", "Burned"].min()[["NonBurned", "Burned"]]
            Aux = (SummaryDF["AVGNonBurned"] + SummaryDF["AVGBurned"] + SummaryDF["AVGFirebreak"])
            SummaryDF["%AVGNonBurned"], SummaryDF["%AVGBurned"], SummaryDF["%AVGFirebreak"] = SummaryDF["AVGNonBurned"] / Aux, SummaryDF["AVGBurned"] / Aux, SummaryDF["AVGFirebreak"] / Aux
            SummaryDF.reset_index(inplace=True)
            SummaryDF.to_csv(os.path.join(self._StatsFolder, "HourlySummaryAVG.csv"), header=True,
                             index=False, columns=SummaryDF.columns, float_format='%.3f')
            if self._verbose:
                print("Summary DF:\n", SummaryDF)

        # Hourly BoxPlots
        if self._boxPlot:
            self.BoxPlot(Ah, yy="Burned", ylab="# Burned Cells", pal="Reds", title="Burned Cells evolution",
                         Path=self._StatsFolder, namePlot="BurnedCells_BoxPlot", swarm=False)
            self.BoxPlot(Ah, yy="NonBurned", ylab="# NonBurned Cells", pal="Greens", title="NonBurned Cells evolution",
                         Path=self._StatsFolder, namePlot="NonBurnedCells_BoxPlot", swarm=False)
            self.BoxPlot(Ah, yy="Firebreak", ylab="# Firebreak Cells", pal="Blues", title="Firebreak Cells evolution",
                         Path=self._StatsFolder, namePlot="FirebreakCells_BoxPlot", swarm=False)


        # Hourly histograms
        all_zeros = not Ah["Burned"].any()
        if self._histograms is True and self._nSims > 1 and all_zeros is False:
            self.plotHistogram(Ah, NonBurned=True, xx="Hour", xmax=6, KDE=True,
                               title="Histogram: Burned and NonBurned Cells",
                               Path=self._StatsFolder, namePlot="Densities")

'''
    def mergePlot(self, multip=True):
        # Read Forest
        ForestFile = os.path.join( self._OutFolder, "InitialForest.png")
        p1 = np.asarray( open_image( ForestFile))
        p1 = add_alpha_channel(p1)
        # Stats per simulation
        for i in tqdm(range(self._nSims)):
            PlotPath = os.path.join(self._OutFolder, "Plots", "Plots" + str(i + 1))
            PlotFiles = glob.glob(os.path.join(PlotPath, 'Fire[0-9]*.*'))

            if multip is False:
                for (j, _) in enumerate(PlotFiles):
                    self.combinePlot( p1, self._OutFolder, j + 1, i + 1)  
            
            else:
                if system()!='Windows':
                    # Multiprocess
                    jobs = []

                    for (j, _) in enumerate(PlotFiles):
                        p = Process(target=self.combinePlot, args=( p1, self._OutFolder, j + 1, i + 1,))
                        jobs.append(p)
                        p.start()

                    # complete the processes
                    for job in jobs:
                        job.join()            
                else:
                    # MonopSerial
                    for (j, _) in enumerate(PlotFiles):
                        self.combinePlot( p1, self._OutFolder, j + 1, i + 1)  

    def ReadLogfile(self,folder,word_to_be_found):
        a_file = open(os.path.join(folder,"Logfile.txt"), "r")
        list_of_values = []
        simulations=[]
        for line in a_file:
            stripped_line = line.strip()
            line_list = stripped_line.split()
            if word_to_be_found in line_list:
                word_index=line_list.index(word_to_be_found)
                simulation=(line_list[word_index+1])[:-1]
                value=line_list[word_index+2]#valor subsiguiente a sim
                list_of_values.append(int(value))
                simulations.append(int(simulation))
        a_file.close()
        return list_of_values,simulations
    
    def ObtainIgnitionPairs(self,ncols,ignitions):
        rows=[]
        cols=[]
        for ignition in ignitions:
            row=int(ignition/ncols)+1
            col=ignition%ncols
            rows.append(row)
            cols.append(col)
        return rows,cols
    
    #generates a txt representing the ignition in each simulation. the format is: simulation, ordinal point, row, column
    def OutfileIgnitions(self):
        list_of_values,simulations=self.ReadLogfile(self._OutFolder, "sim")
        rows,cols=self.ObtainIgnitionPairs(self._Cols, list_of_values)
        dict_df={"Simulation": simulations,"Cell":list_of_values ,"Row": rows,"Col":cols}
        df=pd.DataFrame(dict_df)
        df.sort_values("Simulation",inplace=True)
        np.savetxt(os.path.join(self._OutFolder,'Ignition_Coordinates.txt'), df.values, fmt='%d')
        #df.to_csv(os.path.join(results_folder,"Ignition_Coordinates.csv"),index=False)
'''
