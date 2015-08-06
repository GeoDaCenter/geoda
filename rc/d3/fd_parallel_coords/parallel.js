var width = 400,
    height = 400;

var color = d3.scale.category20();

var line = d3.svg.line()
  .x(function(d) { return d.x; })
  .y(function(d) { return d.y; });

var axis = d3.svg.axis().orient("left");

var x = d3.scale.linear().domain([0,width]).range([width-30,30]),
    y = d3.scale.linear().domain([0,height]).range([30,height-30])
    groups = d3.scale.linear().domain([0,10]).range([30,height-30]);

var force = d3.layout.force()
    .charge(-120)
    .linkDistance(30)
    .friction(0.8)
    .size([width, height]);

var right = d3.select("#chart-right").append("svg")
    .attr("width", width)
    .attr("height", height);

var left = d3.select("#chart-left").append("svg")
    .attr("width", width)
    .attr("height", height);

d3.json("miserables.json", function(json) {
  force
      .nodes(json.nodes)
      .links(json.links)
      .start();

  var lines = left.selectAll("path.node")
      .data(json.nodes, function(d) { return d.name })
    .enter().append("path")
      .attr("class", "node")
      .style("stroke", function(d) { return color(d.group); });

  // Add an axis and title.
  var g = left.selectAll("g.trait")
      .data(['x','y','groups'])
    .enter().append("svg:g")
      .attr("class", "trait")
      .attr("transform", function(d,i) { return "translate(" + (40+160*i) + ")"; })
  g.append("svg:g")
      .attr("class", "axis")
      .each(function(d) { d3.select(this).call(axis.scale(window[d])); })
    .append("svg:text")
      .attr("class", "title")
      .attr("text-anchor", "middle")
      .attr("y", 12)
      .text(String);

  var link = right.selectAll("line.link")
      .data(json.links)
    .enter().append("line")
      .attr("class", "link")
      .style("stroke-width", function(d) { return Math.sqrt(d.value); });

  var circles = right.selectAll("circle.node")
      .data(json.nodes, function(d) { return d.name })
    .enter().append("circle")
      .attr("class", "node")
      .attr("r", 5)
      .style("fill", function(d) { return color(d.group); })
      .on("mouseover", function() {
        d3.selectAll("path.node")
          .data(d3.select(this).data())
          .style("stroke-width", 5)
          .style("stroke", function(d) { return color(d.group); });
        d3.select(this).attr('r', 12);
      })
      .on("mouseout", function() {
        d3.selectAll("path.node")
          .data(d3.select(this).data())
          .style("stroke-width", null)
          .style("stroke", function(d) { return color(d.group); });
        d3.select(this).attr('r', 5);
      })
      .call(force.drag);

  var circles = right.selectAll("circle.node")
      .data(json.nodes, function(d) { return d.name })

  circles.append("title")
      .text(function(d) { return d.name; });
  force.on("tick", function() {
    lines.attr("d", function(d,i) {
      return line([
        {x:40, y:x(d.x)},
        {x:200, y:y(d.y)},
        {x:360, y:groups(d.group)}
      ]);
    });
    link.attr("x1", function(d) { return d.source.x; })
        .attr("y1", function(d) { return d.source.y; })
        .attr("x2", function(d) { return d.target.x; })
        .attr("y2", function(d) { return d.target.y; });
    circles.attr("cx", function(d) { return d.x; })
        .attr("cy", function(d) { return d.y; });
  });
});

