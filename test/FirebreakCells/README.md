# Firebreak Cells Option
Cell2Fire is able to incorporate a harvest plan into the simulating process using --FirebreakCells. 
The indicated cells will now be considered as non-fuel.
In test folder you can find a dedicated instance to execute this module.

# Usage
The given example harvest_plan.csv shows the structure of the harvest plan input.
The cells coordenates must exist whitin the given instance or the firebreaks will not work.

Example command line for using test instance:
./Cell2Fire --input-instance-folder ../test/FirebreakCells/ --output-folder ../test/FirebreakCells/results/ --nsims 5 --output-messages --final-grid --sim K --ignition-random --FirebreakCells ../test/FirebreakCells/harvest_plan.csv
